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

#include <string>
#include <vector>

typedef TestDataNDC<std::string> ItemNDC;

namespace
{
  typedef etl::intrusive_avl_tree_base<0>::link_type ZeroLink;
  typedef etl::intrusive_avl_tree_base<1>::link_type FirstLink;

  //***************************************************************************
  class ItemNDCNode : public ZeroLink, public FirstLink
  {
  public:

    ItemNDCNode(const std::string& text, const int index = 0)
      : data(text, index)
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

    ItemNDC data;
  };

  //***************************************************************************
  struct CompareItemNDCNode
  {
    bool operator ()(const ItemNDCNode& lhs, const ItemNDCNode& rhs) const
    {
      return lhs < rhs;
    }
  };

  //***************************************************************************
  typedef etl::intrusive_avl_tree<ItemNDCNode> DataNDC0;
  typedef etl::intrusive_avl_tree<ItemNDCNode, 1> DataNDC1;

  typedef std::vector<ItemNDCNode> InitialDataNDC;

  SUITE(test_intrusive_avl_tree)
  {
    InitialDataNDC sorted_data;

    //*************************************************************************
    struct SetupFixture
    {
      SetupFixture()
      {
        sorted_data = { ItemNDCNode("0"), ItemNDCNode("1"), ItemNDCNode("2"), ItemNDCNode("3"), ItemNDCNode("4"), ItemNDCNode("5"), ItemNDCNode("6"), ItemNDCNode("7"), ItemNDCNode("8"), ItemNDCNode("9") };
      }

      // static int always_less(const Data&) { return -1; }
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
      DataNDC0 data0(sorted_data.begin(), sorted_data.end(), CompareItemNDCNode());

      bool are_equal = std::equal(data0.begin(), data0.end(), sorted_data.begin());
      CHECK(are_equal);

      are_equal = std::equal(
        etl::reverse_iterator<DataNDC0::iterator>(data0.end()),
        etl::reverse_iterator<DataNDC0::iterator>(data0.begin()),
        sorted_data.rbegin());
      CHECK(are_equal);
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_const_iterator)
    {
      const DataNDC0 data0(sorted_data.begin(), sorted_data.end(), CompareItemNDCNode());

      bool are_equal = std::equal(data0.begin(), data0.end(), sorted_data.begin());
      CHECK(are_equal);

      are_equal = std::equal(
        etl::reverse_iterator<DataNDC0::const_iterator>(data0.cend()),
        etl::reverse_iterator<DataNDC0::const_iterator>(data0.cbegin()),
        sorted_data.rbegin());
      CHECK(are_equal);
    }

    //*************************************************************************
    TEST_FIXTURE(SetupFixture, test_xxx)
    {
      // etl::intrusive_avl_tree<Data> tree;
      // CHECK(tree.begin() == tree.end());
      //
      // Data data1(1);
      //
      // CHECK(tree.empty());
      //
      // const auto ptr_modified = tree.find_or_create(
      //   Data::always_less, [&data1] { return &data1; });
      //
      // CHECK_EQUAL(&data1, ptr_modified.first);
      // CHECK(ptr_modified.second);
      //
      // CHECK(!tree.empty());
    }
  }

}  // namespace
