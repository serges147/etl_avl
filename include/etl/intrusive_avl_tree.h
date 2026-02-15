///\file

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

#ifndef ETL_INTRUSIVE_AVL_TREE_INCLUDED
#define ETL_INTRUSIVE_AVL_TREE_INCLUDED

#include "platform.h"
#include "type_traits.h"
#include "error_handler.h"
#include "intrusive_links.h"
#include "iterator.h"
#include "utility.h"

#include <stddef.h>

namespace etl
{
  //***************************************************************************
  /// Exception for the intrusive_avl_tree.
  /// \ingroup intrusive_avl_tree
  //***************************************************************************
  class intrusive_avl_tree_exception : public exception
  {
  public:

    intrusive_avl_tree_exception(string_type reason_, string_type file_name_, numeric_type line_number_)
      : exception(reason_, file_name_, line_number_)
    {
    }
  };

  //***************************************************************************
  /// Iterator exception for the intrusive_avl_tree.
  /// \ingroup intrusive_avl_tree
  //***************************************************************************
  class intrusive_avl_tree_iterator_exception : public intrusive_avl_tree_exception
  {
  public:

    intrusive_avl_tree_iterator_exception(string_type file_name_, numeric_type line_number_)
      : intrusive_avl_tree_exception(ETL_ERROR_TEXT("intrusive_avl_tree:iterator", ETL_INTRUSIVE_AVL_TREE_FILE_ID"A"), file_name_, line_number_)
    {
    }
  };

  //***************************************************************************
  /// \ingroup intrusive_avl_tree
  /// Base for intrusive AVL tree. Stores elements derived from 'intrusive_avl_tree_base<ID>::link_type'.
  /// \tparam ID_ The link ID that the value is derived from.
  //***************************************************************************
  template <size_t ID_>
  class intrusive_avl_tree_base
  {
  public:
    enum
    {
      ID = ID_,
    };

    /// Base for elements of this AVL tree.
    struct link_type : private etl::tree_link<ID_>
    {
      link_type()
        : etl::tree_link<ID_>()
        , etl_bf(0)
      {
      }

    private:
      typedef etl::tree_link<ID_> base;
      friend class intrusive_avl_tree_base;

      bool is_origin() const
      {
        return base::etl_parent == ETL_NULLPTR;
      }

      link_type* get_parent()
      {
        return static_cast<link_type*>(base::etl_parent);
      }

      const link_type* get_parent() const
      {
        return static_cast<const link_type*>(base::etl_parent);
      }

      bool is_child(const bool is_right) const
      {
        const link_type* parent = get_parent();
        return (ETL_NULLPTR != parent) && (this == parent->get_child(is_right));
      }

      link_type* get_child(const bool is_right)
      {
        return static_cast<link_type*>(is_right ? base::etl_right : base::etl_left);
      }

      const link_type* get_child(const bool is_right) const
      {
        return static_cast<const link_type*>(is_right ? base::etl_right : base::etl_left);
      }

      base*& get_child_ref(const bool is_right)
      {
        return is_right ? base::etl_right : base::etl_left;
      }

      int8_t etl_bf; ///< Stores -1, 0 or +1 balancing factor.

    };  // link_type

    //*************************************************************************
    /// Checks if the tree is in the empty state.
    //*************************************************************************
    bool empty() const
    {
      return get_root() == ETL_NULLPTR;
    }

  protected:

    //*************************************************************************
    /// Constructor
    //*************************************************************************
    intrusive_avl_tree_base()
    {
    }

    //*************************************************************************
    /// Destructor
    //*************************************************************************
    ~intrusive_avl_tree_base()
    {
    }

    link_type* get_root()
    {
      return static_cast<link_type*>(origin.etl_left);
    }

    const link_type* get_root() const
    {
      return static_cast<const link_type*>(origin.etl_left);
    }

    link_type& get_origin()
    {
      return origin;
    }

    const link_type& get_origin() const
    {
      return origin;
    }

    template <typename TLink>
    static TLink* begin_impl(TLink& origin)
    {
      TLink* curr = &origin;
      TLink* next = curr->get_child(false);
      while (next != ETL_NULLPTR)
      {
        curr = next;
        next = next->get_child(false);
      }
      return curr;
    }

    template <typename TLink>
    static TLink* end_impl(TLink& origin)
    {
      return &origin;
    }

    template <typename TLink>
    static TLink* find_extremum_impl(TLink* curr, const bool is_max)
    {
      TLink* next = curr->get_child(is_max);
      while (ETL_NULLPTR != next)
      {
        curr = next;
        next = curr->get_child(is_max);
      }
      return curr;
    }

    template <typename TLink>
    static TLink* next_in_order_impl(TLink* curr)
    {
      if ((ETL_NULLPTR == curr) || curr->is_origin())
      {
        return curr;
      }

      if (TLink* const next = curr->get_child(true))
      {
        return find_extremum_impl(next, false);
      }

      while (curr->is_child(true))
      {
        curr = curr->get_parent();
      }
      return curr->get_parent();
    }

    template <typename TLink>
    static TLink* prev_in_order_impl(TLink* curr)
    {
      if (ETL_NULLPTR == curr)
      {
        return curr;
      }

      if (TLink* const next = curr->get_child(false))
      {
        return find_extremum_impl(next, true);
      }

      while (curr->is_child(false))
      {
        curr = curr->get_parent();
      }
      return curr->is_origin() ? curr : curr->get_parent();
    }

    template <typename TValue, typename TIterator, typename TLessComp>
    void assign_impl(TIterator first, TIterator last, const TLessComp& lessComp)
    {
      #if ETL_IS_DEBUG_BUILD
      const intmax_t diff = etl::distance(first, last);
      ETL_ASSERT(diff >= 0, ETL_ERROR(intrusive_avl_tree_iterator_exception));
      #endif

      // Add all the elements.
      while (first != last)
      {
        link_type& link = *first++;
        TValue& value = static_cast<TValue&>(link);
        const CompareFactory<TValue, TLessComp> compareFactory(value, lessComp);
        find_or_insert_impl<TValue>(compareFactory, compareFactory);
      }
    }

    template <typename TValue, typename TLink, typename TCompare>
    static TValue* find_impl(TLink* const root, const TCompare& comp)
    {
      // Try to find existing node.
      TLink* curr = root;
      while (ETL_NULLPTR != curr)
      {
        TValue* const result = static_cast<TValue*>(curr);
        const int cmp = comp(*result);
        if (0 == cmp)
        {
          // Found!
          return result;
        }

        curr = curr->get_child(cmp > 0);
      }

      // Not found.
      return ETL_NULLPTR;
    }

    template <typename TValue, typename TCompare, typename TFactory>
    etl::pair<TValue*, bool> find_or_insert_impl(const TCompare& comp, const TFactory& factory)
    {
      // Try to find existing node.
      bool is_right = false;
      link_type* curr = get_root();
      link_type* parent = &origin;
      while (ETL_NULLPTR != curr)
      {
        TValue* const result = static_cast<TValue*>(curr);
        const int cmp = comp(*result);
        if (0 == cmp)
        {
          // Found! Tree was not modified.
          return etl::make_pair(result, false);
        }

        parent = curr;
        is_right = cmp > 0;
        curr = curr->get_child(is_right);
      }

      // Try to instantiate new node.
      TValue* const result = factory();
      if (ETL_NULLPTR == result)
      {
        // Failed (or rejected)! The tree was not modified.
        return etl::make_pair(ETL_NULLPTR, false);
      }

      // Link the new node.
      if (parent != &origin)
      {
        parent->get_child_ref(is_right) = result;
      }
      else
      {
        origin.etl_left = result;
      }
      static_cast<link_type*>(result)->etl_parent = parent;

      // TODO: Rebalance the tree.

      // Successfully linked, so the tree was modified.
      return etl::make_pair(result, true);
    }

  private:
    link_type origin; ///< This is the origin link which left child points to the root link of the tree.

    template <typename TValue, typename TLessComp>
    struct CompareFactory
    {
      TValue& value;
      const TLessComp& lessComp;

      CompareFactory(TValue& valueRef, const TLessComp& lessCompRef)
        : value(valueRef)
        , lessComp(lessCompRef) {}

      /// Adopts `less` comparator to "integer" one.
      int operator()(const TValue& other) const
      {
        return lessComp(value, other) ? -1 : +1;
      }

      /// Fake "factory" that returns address of existing value.
      TValue* operator()() const
      {
        return &value;
      }

    };  // CompareFactory

  };  // intrusive_avl_tree_base

  //***************************************************************************
  /// \ingroup intrusive_avl_tree
  /// An intrusive AVL tree. Stores elements derived from 'intrusive_avl_tree<ID>::link_type'.
  /// \warning This tree cannot be used for concurrent access from multiple threads.
  /// \tparam TValue The type of value that the tree holds.
  /// \tparam ID_ The link ID that the value is derived from.
  //***************************************************************************
  template <typename TValue, size_t ID_ = 0>
  class intrusive_avl_tree : public etl::intrusive_avl_tree_base<ID_>
  {
    typedef etl::intrusive_avl_tree_base<ID_> base;

  public:

    // Node typedef.
    typedef typename base::link_type link_type;

    // STL style typedefs.
    typedef TValue            value_type;
    typedef value_type*       pointer;
    typedef const value_type* const_pointer;
    typedef value_type&       reference;
    typedef const value_type& const_reference;
    typedef size_t            size_type;


    //*************************************************************************
    /// iterator.
    //*************************************************************************
    class iterator : public etl::iterator<ETL_OR_STD::bidirectional_iterator_tag, value_type>
    {
    public:

      friend class intrusive_avl_tree;
      friend class const_iterator;

      iterator()
        : p_value(ETL_NULLPTR)
      {
      }

      iterator(const iterator& other)
        : p_value(other.p_value)
      {
      }

      iterator& operator ++()
      {
        p_value = base::next_in_order_impl(p_value);
        return *this;
      }

      iterator operator ++(int)
      {
        iterator temp(*this);
        p_value = base::next_in_order_impl(p_value);
        return temp;
      }

      iterator& operator --()
      {
        p_value = base::prev_in_order_impl(p_value);
        return *this;
      }

      iterator operator --(int)
      {
        iterator temp(*this);
        p_value = base::prev_in_order_impl(p_value);
        return temp;
      }

      iterator& operator =(const iterator& other)
      {
        p_value = other.p_value;
        return *this;
      }

      reference operator *() const
      {
#include "private/diagnostic_null_dereference_push.h"
        return *static_cast<pointer>(p_value);
#include "private/diagnostic_pop.h"
      }

      pointer operator &() const
      {
        return static_cast<pointer>(p_value);
      }

      pointer operator ->() const
      {
        return static_cast<pointer>(p_value);
      }

      friend bool operator == (const iterator& lhs, const iterator& rhs)
      {
        return lhs.p_value == rhs.p_value;
      }

      friend bool operator != (const iterator& lhs, const iterator& rhs)
      {
        return !(lhs == rhs);
      }

    private:

      iterator(link_type* value)
        : p_value(value)
      {
      }

      link_type* p_value;

    };  // iterator

    //*************************************************************************
    /// const_iterator
    //*************************************************************************
    class const_iterator : public etl::iterator<ETL_OR_STD::bidirectional_iterator_tag, const value_type>
    {
    public:

      friend class intrusive_avl_tree;

      const_iterator()
        : p_value(ETL_NULLPTR)
      {
      }

      const_iterator(const iterator& other)
        : p_value(other.p_value)
      {
      }

      const_iterator(const const_iterator& other)
        : p_value(other.p_value)
      {
      }

      const_iterator& operator ++()
      {
        p_value = base::next_in_order_impl(p_value);
        return *this;
      }

      const_iterator operator ++(int)
      {
        const_iterator temp(*this);
        p_value = base::next_in_order_impl(p_value);
        return temp;
      }

      const_iterator& operator --()
      {
        p_value = base::prev_in_order_impl(p_value);
        return *this;
      }

      const_iterator operator --(int)
      {
        const_iterator temp(*this);
        p_value = base::prev_in_order_impl(p_value);
        return temp;
      }

      const_iterator& operator =(const const_iterator& other)
      {
        p_value = other.p_value;
        return *this;
      }

      const_reference operator *() const
      {
        return *static_cast<const_pointer>(p_value);
      }

      const_pointer operator &() const
      {
        return static_cast<const_pointer>(p_value);
      }

      const_pointer operator ->() const
      {
        return static_cast<const_pointer>(p_value);
      }

      friend bool operator == (const const_iterator& lhs, const const_iterator& rhs)
      {
        return lhs.p_value == rhs.p_value;
      }

      friend bool operator != (const const_iterator& lhs, const const_iterator& rhs)
      {
        return !(lhs == rhs);
      }

    private:

      const_iterator(const link_type* value)
        : p_value(value)
      {
      }

      const link_type* p_value;

    };  // const_iterator

    //*************************************************************************
    /// Constructor
    //*************************************************************************
    intrusive_avl_tree()
      : intrusive_avl_tree_base<ID_>()
    {
    }

    //*************************************************************************
    /// Constructor from range
    //*************************************************************************
    template <typename TIterator, typename TLessComp>
    intrusive_avl_tree(TIterator first, TIterator last, const TLessComp& lessComp, typename etl::enable_if<!etl::is_integral<TIterator>::value, int>::type = 0)
    {
      base::template assign_impl<value_type>(first, last, lessComp);
    }

    //*************************************************************************
    /// Gets the beginning of the intrusive_avl_tree.
    //*************************************************************************
    iterator begin()
    {
      return iterator(base::begin_impl(base::get_origin()));
    }

    //*************************************************************************
    /// Gets the beginning of the intrusive_avl_tree.
    //*************************************************************************
    const_iterator begin() const
    {
      return const_iterator(base::begin_impl(base::get_origin()));
    }

    //*************************************************************************
    /// Gets the beginning of the intrusive_avl_tree.
    //*************************************************************************
    const_iterator cbegin() const
    {
      return begin();
    }

    //*************************************************************************
    /// Gets the end of the intrusive_avl_tree.
    //*************************************************************************
    iterator end()
    {
      return iterator(base::end_impl(base::get_origin()));
    }

    //*************************************************************************
    /// Gets the end of the intrusive_avl_tree.
    //*************************************************************************
    const_iterator end() const
    {
      return const_iterator(base::end_impl(base::get_origin()));
    }

    //*************************************************************************
    /// Gets the end of the intrusive_avl_tree.
    //*************************************************************************
    const_iterator cend() const
    {
      return end();
    }

    template <typename TCompare>
    iterator find(const TCompare& comp)
    {
      pointer ptr = base::template find_impl<value_type>(base::get_root(), comp);
      return make_iterator(ptr, end());
    }

    template <typename TCompare>
    const_iterator find(const TCompare& comp) const
    {
      const_pointer ptr = base::template find_impl<const value_type>(base::get_root(), comp);
      return make_iterator(ptr, end());
    }

    template <typename TCompare, typename TFactory>
    etl::pair<iterator, bool> find_or_insert(const TCompare& comp, const TFactory& factory)
    {
      const etl::pair<value_type*, bool> ptr_mod = base::template find_or_insert_impl<value_type>(comp, factory);
      return etl::make_pair(make_iterator(ptr_mod.first, end()), ptr_mod.second);
    }

  private:

    // Disable copy construction and assignment.
    intrusive_avl_tree(const intrusive_avl_tree&);
    intrusive_avl_tree& operator = (const intrusive_avl_tree& rhs);

    template <typename TIterator, typename Pointer>
    static TIterator make_iterator(Pointer const ptr, const TIterator end)
    {
      return (ptr != ETL_NULLPTR) ? TIterator(ptr) : end;
    }

  };  // intrusive_avl_tree

}  // namespace etl

#endif
