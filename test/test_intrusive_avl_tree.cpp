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

#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

typedef TestDataNDC<int> ItemNDC;

namespace
{
  typedef etl::intrusive_avl_tree_base<0>::link_type ZeroLink;
  typedef etl::intrusive_avl_tree_base<1>::link_type FirstLink;

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
      int operator ()(const ItemNDCNode& other) const
      {
        return value - other.data.value;
      }
    };
    static int always_after(const ItemNDCNode&) { return +1; }
    static int always_before(const ItemNDCNode&) { return -1; }

    ItemNDC data;
  };

  //***************************************************************************
  typedef etl::intrusive_avl_tree<ItemNDCNode> DataNDC0;
  typedef etl::intrusive_avl_tree<ItemNDCNode, 1> DataNDC1;

  typedef std::vector<ItemNDCNode> InitialDataNDC;

  SUITE(test_intrusive_avl_tree)
  {
    //*************************************************************************
    struct SetupFixture
    {
      InitialDataNDC sorted_data;
      InitialDataNDC unsorted_data;

      SetupFixture()
      {
        sorted_data.clear();
        for (int i = 0; i < 31; ++i)
        {
          sorted_data.emplace_back(i, i);
        }

        constexpr std::array<size_t, 31> unsorted_order{
          1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14,
          17, 16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 30, 29, 28,
        };
        unsorted_data.clear();
        for (const auto idx : unsorted_order)
        {
          unsorted_data.emplace_back(sorted_data.at(idx));
        }
      }

      template <typename Tree>
      std::string to_graphviz(Tree& tree) const
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
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_empty_begin_end)
    {
      DataNDC0 data0;

      CHECK(data0.begin() == data0.end());

      const DataNDC0::const_iterator begin = data0.begin();
      const DataNDC0::const_iterator end   = data0.end();
      CHECK(begin == end);

      CHECK(data0.cbegin() == data0.cend());
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_iterator)
    {
      DataNDC0 data0(sorted_data.begin(), sorted_data.end(), std::less<ItemNDCNode>());
      std::cout << to_graphviz(data0);

      bool are_equal = std::equal(data0.begin(), data0.end(), sorted_data.begin());
      CHECK(are_equal);

      are_equal = std::equal(
        etl::reverse_iterator<DataNDC0::iterator>(data0.end()),
        etl::reverse_iterator<DataNDC0::iterator>(data0.begin()),
        sorted_data.rbegin());
      CHECK(are_equal);

      auto curr = data0.begin();
      CHECK(curr);
      CHECK(curr.has_value());
      const auto& front = *curr;
      CHECK_EQUAL(&front, &curr);
      CHECK_EQUAL(front.data.value, sorted_data.front().data.value);
      CHECK_EQUAL(curr->data.value, sorted_data.front().data.value);
      auto prev = curr++;
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
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_iterator_default)
    {
      DataNDC0::iterator it;
      CHECK_EQUAL(false, static_cast<bool>(it));
      CHECK_EQUAL(false, it.has_value());

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
      const DataNDC0 data0(unsorted_data.begin(), unsorted_data.end(), std::less<ItemNDCNode>());
      std::cout << to_graphviz(data0);

      bool are_equal = std::equal(data0.begin(), data0.end(), sorted_data.begin());
      CHECK(are_equal);

      are_equal = std::equal(
        etl::reverse_iterator<DataNDC0::const_iterator>(data0.cend()),
        etl::reverse_iterator<DataNDC0::const_iterator>(data0.cbegin()),
        sorted_data.rbegin());
      CHECK(are_equal);

      auto curr = data0.begin();
      CHECK(curr);
      CHECK(curr.has_value());
      const auto& front = *curr;
      CHECK_EQUAL(&front, &curr);
      CHECK_EQUAL(front.data.value, sorted_data.front().data.value);
      CHECK_EQUAL(curr->data.value, sorted_data.front().data.value);
      auto prev = curr++;
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
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_const_iterator_default)
    {
      DataNDC0::const_iterator it;
      CHECK_EQUAL(false, static_cast<bool>(it));
      CHECK_EQUAL(false, it.has_value());

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
      CHECK(iterator == data0.end());

      iterator = data0.find(ItemNDCNode::always_after);
      CHECK(iterator == data0.end());

      iterator = data0.find(ItemNDCNode::CompareByValue{5});
      CHECK(iterator != data0.end());
      CHECK_EQUAL(iterator->data, sorted_data[5].data);
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_find_const)
    {
      const DataNDC0 data0(sorted_data.begin(), sorted_data.end(), std::less<ItemNDCNode>());

      auto iterator = data0.find(ItemNDCNode::always_before);
      CHECK(iterator == data0.end());

      iterator = data0.find(ItemNDCNode::always_after);
      CHECK(iterator == data0.end());

      iterator = data0.find(ItemNDCNode::CompareByValue{5});
      CHECK(iterator != data0.end());
      CHECK_EQUAL(iterator->data, sorted_data[5].data);
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

        CHECK(it_mod.second);
        CHECK(it_mod.first != data0.end());
        CHECK_EQUAL(&node0a, &it_mod.first);
      }

      // Find existing.
      {
        const auto it_mod = data0.find_or_insert(
          ItemNDCNode::CompareByValue{0}, [&node0b] { return &node0b; });

        CHECK(!it_mod.second);
        CHECK(it_mod.first != data0.end());
        CHECK_EQUAL(&node0a, &it_mod.first);

        data0.erase(it_mod.first);
      }
    }
  }

}  // namespace
