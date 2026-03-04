/******************************************************************************
The MIT License(MIT)

Embedded Template Library.
https://github.com/ETLCPP/etl
https://www.etlcpp.com

Copyright(c) 2026 Sergei Shirokov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#include "unit_test_framework.h"

#include "data.h"

#include "etl/intrusive_avl_tree.h"
#include "etl/optional.h"

#include <array>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

typedef TestDataM<int> ItemM;
typedef TestDataNDC<int> ItemNDC;

namespace
{
  typedef etl::intrusive_avl_tree_base<0>::link_type ZeroLink;
  typedef etl::intrusive_avl_tree_base<1>::link_type FirstLink;
  typedef etl::intrusive_avl_tree_base<2>::link_type SecondLink;

  //***************************************************************************
  class ItemNDCNode : public ZeroLink, public FirstLink
  {
  public:

    ItemNDCNode(const int value, const int index = 0)
      : data(value, index)
    {
    }

    friend bool operator <(const ItemNDCNode& lhs, const ItemNDCNode& rhs)
    {
      return lhs.data < rhs.data;
    }

    friend bool operator ==(const ItemNDCNode& lhs, const ItemNDCNode& rhs)
    {
      return lhs.data == rhs.data;
    }

    struct CompareByValue
    {
      int value;
      etl::optional<etl::exception> excptn;

      explicit CompareByValue(
        const int value_,
        const etl::optional<etl::exception>& excptn_ = etl::nullopt)
        : value(value_)
        , excptn(excptn_)
      {}

      int operator ()(const ItemNDCNode& other) const
      {
        if (excptn.has_value())
        {
          throw etl::exception(excptn.value());
        }
        return value - other.data.value;
      }
    };
    static int always_after(const ItemNDCNode&) { return +1; }
    static int always_before(const ItemNDCNode&) { return -1; }

    ItemNDC data;

  };  // CompareByValue

  //***************************************************************************
  class ItemMNode : public SecondLink
  {
  public:

    explicit ItemMNode(const int value)
      : data(value)
    {
    }

    friend bool operator <(const ItemMNode& lhs, const ItemMNode& rhs)
    {
      return lhs.data < rhs.data;
    }

    friend bool operator ==(const ItemMNode& lhs, const ItemMNode& rhs)
    {
      return lhs.data == rhs.data;
    }

    struct CompareByValue
    {
      int value;
      int operator ()(const ItemMNode& other) const
      {
        return value - other.data.value;
      }
    };

    ItemM data;
  };

  //***************************************************************************
  typedef etl::intrusive_avl_tree<ItemNDCNode> DataNDC0;
  typedef etl::intrusive_avl_tree<ItemNDCNode, 1> DataNDC1;
  typedef etl::intrusive_avl_tree<ItemMNode, 2> DataM;

  typedef std::vector<ItemMNode> InitialDataM;
  typedef std::vector<ItemNDCNode> InitialDataNDC;

  SUITE(test_intrusive_avl_tree)
  {
    //*************************************************************************
    struct SetupFixture
    {
      InitialDataNDC sorted_data;
      InitialDataNDC unsorted_data;
      InitialDataM sorted_data_moveable;

      SetupFixture()
      {
        sorted_data.clear();
        for (int i = 0; i < 31; ++i)
        {
          sorted_data.emplace_back(i, i);
          sorted_data_moveable.emplace_back(i);
        }

        constexpr std::array<size_t, 31> unsorted_order{
          1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14,
          17, 16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 30, 29, 28,
        };
        unsorted_data.clear();
        for (const auto idx : unsorted_order)
        {
          unsorted_data.emplace_back(idx, idx);
        }
      }

      template <typename Iterator>
      int verify_link(const Iterator it) const
      {
        if (!it.has_value())
        {
          return 0;
        }

        const auto parent = it.get_parent();
        const auto left_child = it.get_child(false);
        const auto right_child = it.get_child(true);
        if (parent.has_value())
        {
          const auto parent_left_child = parent.get_child(false);
          const auto parent_right_child = parent.get_child(true);
          CHECK((it == parent_left_child) || (it == parent_right_child));
        }

        const int left_height = verify_link(left_child);
        if (left_child.has_value())
        {
          CHECK(it == left_child.get_parent());
        }

        const int right_height = verify_link(right_child);
        if (right_child.has_value())
        {
          CHECK(it == right_child.get_parent());
        }

        const int8_t balance_factor = it.get_balance_factor();
        CHECK((-1 <= balance_factor) && (balance_factor <= 1));
        CHECK_EQUAL(right_height - left_height, static_cast<int>(balance_factor));
        return 1 + etl::max(right_height, left_height);
      }

      template <typename Tree>
      void verify_tree(const Tree& tree) const
      {
        typedef typename Tree::value_type value_type;
        typedef etl::reverse_iterator<typename Tree::const_iterator> rev_it;

        const auto root = tree.get_root();
        verify_link(root);
        CHECK(etl::is_sorted(tree.begin(), tree.end()));
        CHECK(etl::is_sorted(rev_it(tree.end()), rev_it(tree.begin()), etl::greater<value_type>()));
      }

      template <typename Tree>
      std::string to_graphviz(const Tree& tree) const
      {
        std::ostringstream ss;
        ss << "// \"dot\" engine at https://edotor.net/\n";
        ss << "digraph {\n";
        ss << "node[style=filled,fontcolor=white];\n";

        const auto beg = tree.begin();
        const auto end = tree.end();
        for (auto curr = beg; curr != end; ++curr)
        {
          ss << curr->data.value;
          const auto bf = curr.get_balance_factor();
          const std::string bf_color = (bf == 0) ? "black" : (bf < 0) ? "blue" : "orange";
          ss << "[fillcolor=" << bf_color << "];";
        }
        ss << "\n";
        for (auto curr = beg; curr != end; ++curr)
        {
          if (auto child = curr.get_child(false))
          {
            ss << curr->data.value;
            ss << ":sw->" << child->data.value << ":n;";
          }
          if (auto child = curr.get_child(true))
          {
            ss << curr->data.value;
            ss << ":se->" << child->data.value << ":n;";
          }
        }
        ss << "\n}\n";
        return ss.str();
      }
    };

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_default_constructor)
    {
      DataNDC0 data0;
      DataNDC1 data1;

      CHECK(data0.empty());
      CHECK(data1.empty());

      CHECK(data0.begin() == data0.end());
      CHECK(data1.begin() == data1.end());

      verify_tree(data0);
      verify_tree(data1);
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_constructor_with_bad_iterator)
    {
      auto action = [this]()
      {
        // Note end/begin order -> should throw!
        DataNDC0 data0(sorted_data.end(), sorted_data.begin(), std::less<ItemNDCNode>());
      };
      CHECK_THROW(action(), etl::intrusive_avl_tree_iterator_exception);
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_empty_begin_end_root)
    {
      DataNDC0 data0;

      CHECK(data0.begin() == data0.end());
      CHECK(!data0.get_root().has_value());

      const DataNDC0::const_iterator begin = data0.begin();
      const DataNDC0::const_iterator end   = data0.end();
      CHECK(begin == end);

      CHECK(data0.cbegin() == data0.cend());
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_iterator)
    {
      typedef etl::reverse_iterator<DataNDC0::iterator> rev_it;

      DataNDC0 data0(sorted_data.begin(), sorted_data.end(), std::less<ItemNDCNode>());
      verify_tree(data0);

      bool are_equal = std::equal(data0.begin(), data0.end(), sorted_data.begin());
      CHECK(are_equal);

      are_equal = std::equal(rev_it(data0.end()), rev_it(data0.begin()), sorted_data.rbegin());
      CHECK(are_equal);

      auto curr = data0.begin();
      CHECK(curr);
      CHECK(curr.has_value());
      const auto& front = *curr;
      CHECK_EQUAL(&front, &curr);
      CHECK_EQUAL(front.data.value, sorted_data.front().data.value);
      CHECK_EQUAL(curr->data.value, sorted_data.front().data.value);
      auto prev = curr++;
      CHECK(curr == prev.get_parent());
      CHECK(prev == curr.get_child(false));
      CHECK(curr != data0.begin());
      CHECK(prev == data0.begin());
      CHECK(prev-- == data0.begin());
      CHECK(prev == data0.end());

      curr = data0.end();
      CHECK(curr);
      CHECK(curr.has_value());
      CHECK(curr-- == data0.end());
      CHECK(curr != data0.end());
      CHECK_EQUAL(curr->data.value, sorted_data.back().data.value);

      prev = curr--;
      CHECK(curr == prev.get_parent());
      CHECK(prev == curr.get_child(true));

      curr = data0.get_root();
      CHECK(curr.has_value());
      CHECK_EQUAL(curr->data.value, sorted_data.at(sorted_data.size() / 2).data.value);
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_iterator_default)
    {
      DataNDC0::iterator it;

      CHECK_EQUAL(false, static_cast<bool>(it));
      CHECK_EQUAL(false, it.has_value());

      CHECK_EQUAL(0, it.get_balance_factor());
      CHECK_EQUAL(false, it.get_parent().has_value());
      CHECK_EQUAL(false, it.get_child(true).has_value());
      CHECK_EQUAL(false, it.get_child(false).has_value());

      ++it;
      CHECK_EQUAL(false, static_cast<bool>(it));
      CHECK_EQUAL(false, it.has_value());

      --it;
      CHECK_EQUAL(false, static_cast<bool>(it));
      CHECK_EQUAL(false, it.has_value());
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_const_iterator)
    {
      typedef etl::reverse_iterator<DataNDC0::const_iterator> rev_it;

      const DataNDC0 data0(unsorted_data.begin(), unsorted_data.end(), std::less<ItemNDCNode>());
      verify_tree(data0);
      std::cout << to_graphviz(data0);

      bool are_equal = std::equal(data0.begin(), data0.end(), sorted_data.begin());
      CHECK(are_equal);

      are_equal = std::equal(rev_it(data0.cend()), rev_it(data0.cbegin()), sorted_data.rbegin());
      CHECK(are_equal);

      auto curr = data0.begin();
      CHECK(curr);
      CHECK(curr.has_value());
      const auto& front = *curr;
      CHECK_EQUAL(&front, &curr);
      CHECK_EQUAL(front.data.value, sorted_data.front().data.value);
      CHECK_EQUAL(curr->data.value, sorted_data.front().data.value);
      auto prev = curr++;
      CHECK(curr == prev.get_parent());
      CHECK(prev == curr.get_child(false));
      CHECK(curr != data0.begin());
      CHECK(prev == data0.begin());
      CHECK(prev-- == data0.begin());
      CHECK(prev == data0.end());

      curr = data0.end();
      CHECK(curr);
      CHECK(curr.has_value());
      CHECK(curr-- == data0.end());
      CHECK(curr != data0.end());
      CHECK_EQUAL(curr->data.value, sorted_data.back().data.value);

      prev = curr--;
      CHECK(curr == prev.get_parent());
      CHECK(prev == curr.get_child(true));

      curr = data0.get_root();
      CHECK(curr.has_value());
      CHECK_EQUAL(curr->data.value, sorted_data.at(sorted_data.size() / 2).data.value);
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_const_iterator_default)
    {
      DataNDC0::const_iterator it;
      CHECK_EQUAL(false, static_cast<bool>(it));
      CHECK_EQUAL(false, it.has_value());

      CHECK_EQUAL(0, it.get_balance_factor());
      CHECK_EQUAL(false, it.get_parent().has_value());
      CHECK_EQUAL(false, it.get_child(true).has_value());
      CHECK_EQUAL(false, it.get_child(false).has_value());

      ++it;
      CHECK_EQUAL(false, static_cast<bool>(it));
      CHECK_EQUAL(false, it.has_value());

      --it;
      CHECK_EQUAL(false, static_cast<bool>(it));
      CHECK_EQUAL(false, it.has_value());
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_find)
    {
      DataNDC0 data0(sorted_data.begin(), sorted_data.end(), std::less<ItemNDCNode>());

      auto iterator = data0.find(ItemNDCNode::always_before);
      CHECK(iterator.has_value());
      CHECK(iterator == data0.end());

      iterator = data0.find(ItemNDCNode::always_after);
      CHECK(iterator.has_value());
      CHECK(iterator == data0.end());

      iterator = data0.find(ItemNDCNode::CompareByValue{5});
      CHECK(iterator.has_value());
      CHECK(iterator != data0.end());
      CHECK_EQUAL(iterator->data, sorted_data[5].data);
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_find_const)
    {
      const DataNDC0 data0(sorted_data.begin(), sorted_data.end(), std::less<ItemNDCNode>());

      auto iterator = data0.find(ItemNDCNode::always_before);
      CHECK(iterator.has_value());
      CHECK(iterator == data0.end());

      iterator = data0.find(ItemNDCNode::always_after);
      CHECK(iterator.has_value());
      CHECK(iterator == data0.end());

      iterator = data0.find(ItemNDCNode::CompareByValue{5});
      CHECK(iterator != data0.end());
      CHECK(iterator.has_value());
      CHECK_EQUAL(iterator->data, sorted_data[5].data);
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_find_throwing)
    {
      const DataNDC0 data0(sorted_data.begin(), sorted_data.end(), std::less<ItemNDCNode>());

      const ItemNDCNode::CompareByValue throwing_compare{0, etl::exception("13", __FILE__, __LINE__)};
      CHECK_THROW(data0.find(throwing_compare), etl::exception);
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_find_or_insert)
    {
      DataNDC0 data0;

      ItemNDCNode node0a(0);
      ItemNDCNode node0b(0);

      // Insert new.
      {
        CHECK(data0.empty());
        const auto it_mod = data0.find_or_insert(
          ItemNDCNode::CompareByValue{0}, [&node0a] { return &node0a; });
        CHECK(!data0.empty());
        verify_tree(data0);

        CHECK(it_mod.second);
        CHECK(it_mod.first.has_value());
        CHECK(it_mod.first != data0.end());
        CHECK_EQUAL(&node0a, &it_mod.first);
      }

      // Find existing.
      {
        const auto it_mod = data0.find_or_insert(
          ItemNDCNode::CompareByValue{0}, [&node0b] { return &node0b; });

        CHECK(!it_mod.second);
        CHECK(it_mod.first.has_value());
        CHECK(it_mod.first != data0.end());
        CHECK_EQUAL(&node0a, &it_mod.first);
      }
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_find_or_insert_unsorted)
    {
      DataNDC0 data0;
      verify_tree(data0);

      for (auto& item: unsorted_data)
      {
        data0.find_or_insert(
                  ItemNDCNode::CompareByValue{item.data.value}, [&item] { return &item; });
        verify_tree(data0);
      }
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_find_or_insert_null_factory)
    {
      DataNDC0 data0;

      const auto it_mod = data0.find_or_insert(
        ItemNDCNode::always_after, [] { return ETL_NULLPTR; });
      CHECK(data0.empty());
      verify_tree(data0);

      CHECK(!it_mod.second);
      CHECK(!it_mod.first.has_value());
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_find_or_insert_already_linked)
    {
      DataNDC0 data0;

      ItemNDCNode node0(0);

      auto insert = [&]()
      {
        return data0.find_or_insert(ItemNDCNode::always_after, [&node0] { return &node0; });
      };

      auto it_mod = insert();
      CHECK(!data0.empty());
      verify_tree(data0);

      // Insert the same node again -> should throw.
      {
        CHECK_THROW(insert(), etl::intrusive_avl_tree_value_is_already_linked);
        CHECK(!data0.empty());
        verify_tree(data0);
      }

      // But it's ok erase it first, and then reinsert.
      {
        data0.erase(it_mod.first);
        it_mod = insert();

        CHECK(it_mod.second);
        CHECK(it_mod.first.has_value());
        CHECK(it_mod.first != data0.end());
        CHECK_EQUAL(&node0, &it_mod.first);
      }
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_find_or_insert_throwing)
    {
      DataNDC0 data0(sorted_data.begin(), sorted_data.end(), std::less<ItemNDCNode>());

      // Try throwing factory.
      {
        auto throwing_action = [&]()
        {
          return data0.find_or_insert(ItemNDCNode::always_after, []() -> ItemNDCNode*
          {
            throw etl::exception("123", __FILE__, __LINE__);
          });
        };
        CHECK_THROW(throwing_action(), etl::exception);
        verify_tree(data0);
        CHECK(std::equal(data0.begin(), data0.end(), sorted_data.begin()));
      }

      // Try throwing comparer.
      {
        auto throwing_action = [&]()
        {
          return data0.find_or_insert(
            [](const ItemNDCNode&) -> int { throw etl::exception("321", __FILE__, __LINE__); },
            []() -> ItemNDCNode* { return ETL_NULLPTR; });
        };
        CHECK_THROW(throwing_action(), etl::exception);
        verify_tree(data0);
        CHECK(std::equal(data0.begin(), data0.end(), sorted_data.begin()));
      }
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_erase)
    {
      DataNDC0 data0(sorted_data.begin(), sorted_data.end(), std::less<ItemNDCNode>());
      verify_tree(data0);

      for (const auto& item: unsorted_data)
      {
        const DataNDC0::iterator it = data0.find(ItemNDCNode::CompareByValue{item.data.index});
        CHECK(it != data0.end());

        const DataNDC0::iterator next_it = data0.erase(it);
        verify_tree(data0);
        CHECK(next_it.has_value());
        if (next_it != data0.end())
        {
          const auto& next = *next_it;
          CHECK(item < next);
        }
      }
      CHECK(data0.empty());
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_erase_const_it)
    {
      DataNDC0 data0(sorted_data.begin(), sorted_data.end(), std::less<ItemNDCNode>());
      verify_tree(data0);

      for (const auto& item: unsorted_data)
      {
        const DataNDC0::const_iterator it = data0.find(ItemNDCNode::CompareByValue{item.data.index});
        CHECK(it != data0.end());

        const DataNDC0::iterator next_it = data0.erase(it);
        verify_tree(data0);
        CHECK(next_it.has_value());
        if (next_it != data0.end())
        {
          const auto& next = *next_it;
          CHECK(item < next);
        }
      }
      CHECK(data0.empty());
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_random_insert_and_erase)
    {
      // Deliberately seeded with fixed number, so that if it fails then always in the same way.
      std::mt19937 mte(123);

      InitialDataNDC nodes;
      constexpr size_t N = 256;
      for (size_t i = 0; i < N; ++i)
      {
        nodes.emplace_back(i, i);
      }

      DataNDC0 data0;

      auto makeRandomNumber = [&mte](const size_t n) -> size_t { return mte() % n; };
      auto insert_item = [&](const size_t n)
      {
        const ItemNDCNode::CompareByValue comp{static_cast<int>(n)};
        const auto it = data0.find(comp);
        if (it == data0.end())
        {
          bool factory_was_called = false;
          auto it_mod = data0.find_or_insert(comp, [&]
          {
            factory_was_called = true;
            return &nodes.at(n);
          });
          CHECK(it_mod.second);
          CHECK(factory_was_called);
          CHECK_EQUAL(n, it_mod.first->data.index);
        }
        else
        {
          CHECK_EQUAL(n, it->data.index);
          auto it_mod = data0.find_or_insert(comp, [&]
          {
            CHECK_MESSAGE("Should no be called!")
            CHECK(false);
            return ETL_NULLPTR;
          });
          CHECK(it == it_mod.first);
          CHECK_FALSE(it_mod.second);
        }
      };
      auto erase_item = [&](const size_t n)
      {
        const ItemNDCNode::CompareByValue comp{static_cast<int>(n)};
        auto it = data0.find(comp);
        if (it != data0.end())
        {
          CHECK_EQUAL(n, it->data.index);
          data0.erase(it);
          it = data0.find(comp);
          CHECK(it == data0.end());
        }
      };

      for (size_t i = 0; i < 10000; ++i)
      {
        if (makeRandomNumber(2) == 0)
        {
          erase_item(makeRandomNumber(N));
        }
        else
        {
          insert_item(makeRandomNumber(N));
        }

        verify_tree(data0);
      }
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_move_item)
    {
      const DataM data0(sorted_data_moveable.begin(), sorted_data_moveable.end(), std::less<ItemMNode>());
      verify_tree(data0);

      const ItemMNode min_item{std::move(sorted_data_moveable.front())};
      CHECK(min_item.data.valid);
      CHECK_FALSE(sorted_data_moveable.front().data.valid);
      CHECK(min_item.data.value == 0);
      auto it = data0.begin();
      CHECK(&it == &min_item);
      verify_tree(data0);

      const ItemMNode max_item{std::move(sorted_data_moveable.back())};
      CHECK(max_item.data.value == static_cast<int>(sorted_data_moveable.size() - 1));
      it = --data0.end();
      CHECK(&it == &max_item);
      verify_tree(data0);

      auto item_idx = static_cast<int>(sorted_data_moveable.size() / 2);
      const ItemMNode root_item{std::move(sorted_data_moveable.at(item_idx))};
      CHECK_EQUAL(root_item.data.value, item_idx);
      it = data0.get_root();
      CHECK(it.has_value());
      CHECK(&it == &root_item);
      verify_tree(data0);

      item_idx = 11;
      const ItemMNode item{std::move(sorted_data_moveable.at(item_idx))};
      CHECK(item.data.value == item_idx);
      it = data0.find(ItemMNode::CompareByValue{item_idx});
      CHECK(&it == &item);
      verify_tree(data0);
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_constructor_move)
    {
      DataM data0a(sorted_data_moveable.begin(), sorted_data_moveable.end(), std::less<ItemMNode>());
      const DataM data0b(std::move(data0a));
      verify_tree(data0b);
      CHECK(std::equal(data0b.begin(), data0b.end(), sorted_data_moveable.begin()));
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_clear)
    {
      DataM data0a(sorted_data_moveable.begin(), sorted_data_moveable.end(), std::less<ItemMNode>());
      data0a.clear();
      verify_tree(data0a);
      CHECK(data0a.empty());
    }
  }

}  // namespace
