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
#include "error_handler.h"
#include "intrusive_links.h"
#include "iterator.h"
#include "memory.h"
#include "type_traits.h"
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
    intrusive_avl_tree_exception(const string_type reason_, const string_type file_name_, const numeric_type line_number_)
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
    intrusive_avl_tree_iterator_exception(const string_type file_name_, const numeric_type line_number_)
      : intrusive_avl_tree_exception(ETL_ERROR_TEXT("intrusive_avl_tree:iterator", ETL_INTRUSIVE_AVL_TREE_FILE_ID "A"), file_name_, line_number_)
    {
    }
  };

  //***************************************************************************
  /// Already exception for the intrusive_avl_tree.
  ///\ingroup intrusive_avl_tree
  //***************************************************************************
  class intrusive_avl_tree_value_is_already_linked : public intrusive_avl_tree_exception
  {
  public:

    intrusive_avl_tree_value_is_already_linked(const string_type file_name_, const numeric_type line_number_)
      : intrusive_avl_tree_exception(ETL_ERROR_TEXT("intrusive_avl_tree:value is already linked", ETL_INTRUSIVE_AVL_TREE_FILE_ID"B"), file_name_, line_number_)
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

#if ETL_USING_CPP11
    intrusive_avl_tree_base(const intrusive_avl_tree_base&) = delete;
    intrusive_avl_tree_base& operator=(const intrusive_avl_tree_base& rhs) = delete;
    intrusive_avl_tree_base& operator=(intrusive_avl_tree_base&& rhs) ETL_NOEXCEPT = delete;
#endif

    /// Base for elements of this AVL tree.
    struct link_type : private etl::tree_link<ID_>
    {
    private:
      typedef etl::tree_link<ID_> base;

    public:
      link_type()
        : base()
        , etl_bf(0)
      {
      }

#if ETL_USING_CPP11
      link_type(link_type&& other) ETL_NOEXCEPT
        : base()
        , etl_bf(0)
      {
        move_impl(other);
      }
#endif

#if ETL_USING_CPP11
      link_type(const link_type&) = delete;
      link_type& operator=(const link_type& rhs) = delete;
      link_type& operator=(link_type&& rhs) ETL_NOEXCEPT = delete;
#endif

      ~link_type()
      {
        // Remove only real and still linked items.
        // Tree itself uses this `link_type` for its `origin` artificial item,
        // but you can't (and there is no need to) erase it.
        if (!is_origin() && is_linked())
        {
          // We can't just remove item (by simple rewiring of links at parent and children)
          // b/c its former "owner" tree has to be rebalanced after the removal -
          // hence the full-blown erasing which might rotate the tree as necessary.
          erase_impl(this);
        }
      }

    private:
      friend class intrusive_avl_tree_base;

      int8_t etl_bf;  ///< Stores -1, 0 or +1 balancing factor.

#if ETL_USING_CPP11
#else
      // Disable copy construction and assignment.
      link_type(const link_type&);
      link_type& operator=(const link_type& rhs);
#endif

      bool is_linked() const
      {
        return base::is_linked();
      }

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

      bool is_left_child() const
      {
        return is_child(false);
      }

      bool is_right_child() const
      {
        return is_child(true);
      }

      link_type* get_child(const bool is_right)
      {
        return static_cast<link_type*>(is_right ? base::etl_right : base::etl_left);
      }

      const link_type* get_child(const bool is_right) const
      {
        return static_cast<const link_type*>(is_right ? base::etl_right : base::etl_left);
      }

      void set_child(link_type* const child, const bool is_right)
      {
        base*& child_ref = is_right ? base::etl_right : base::etl_left;
        child_ref = child;
      }

      link_type* get_left()
      {
        return static_cast<link_type*>(base::etl_left);
      }

      link_type* get_right()
      {
        return static_cast<link_type*>(base::etl_right);
      }

      void move_impl(link_type& other)
      {
        const bool is_right = other.is_right_child();
        base::etl_parent = other.etl_parent;
        base::etl_left = other.etl_left;
        base::etl_right = other.etl_right;
        etl_bf = other.etl_bf;

        other.clear();
        other.etl_bf = 0;

        if (ETL_NULLPTR != base::etl_parent)
        {
          get_parent()->link_child(this, is_right);
        }
        if (ETL_NULLPTR != base::etl_left)
        {
          get_left()->set_parent(this);
        }
        if (ETL_NULLPTR != base::etl_right)
        {
          get_right()->set_parent(this);
        }
      }

      void rotate(const bool is_right)
      {
        const bool       was_right = is_right_child();
        link_type* const leaf = get_child(!is_right);
        etl::link_rotate<base>(this, leaf);
        if (link_type* const parent = leaf->get_parent())
        {
          parent->set_child(leaf, was_right);
        }
      }

      link_type* adjust_balance(const bool increase)
      {
        const int8_t new_bf = etl_bf + (increase ? +1 : -1);
        if ((-1 <= new_bf) && (new_bf <= +1))
        {
          etl_bf = new_bf;
          return this;
        }

        const bool       is_right_rotation = new_bf < 0;
        const int8_t     sign = is_right_rotation ? +1 : -1;
        link_type* const z_leaf = get_child(!is_right_rotation);
        if (z_leaf->etl_bf * sign <= 0)
        {
          rotate(is_right_rotation);
          if (z_leaf->etl_bf == 0)
          {
            etl_bf = -sign;
            z_leaf->etl_bf = +sign;
          }
          else
          {
            etl_bf = 0;
            z_leaf->etl_bf = 0;
          }
          return z_leaf;
        }

        link_type* const y_leaf = z_leaf->get_child(is_right_rotation);
        z_leaf->rotate(!is_right_rotation);
        rotate(is_right_rotation);
        if (y_leaf->etl_bf == 0)
        {
          etl_bf = 0;
          z_leaf->etl_bf = 0;
        }
        else if (y_leaf->etl_bf == sign)
        {
          etl_bf = 0;
          y_leaf->etl_bf = 0;
          z_leaf->etl_bf = -sign;
        }
        else
        {
          etl_bf = +sign;
          y_leaf->etl_bf = 0;
          z_leaf->etl_bf = 0;
        }
        return y_leaf;
      }

      void link_child(link_type* child, const bool is_right)
      {
        if (is_right)
        {
          etl::link_right<base>(this, child);
        }
        else
        {
          etl::link_left<base>(this, child);
        }
      }

    };  // link_type

    //*************************************************************************
    /// Checks if the tree is in the empty state.
    /// Complexity: O(1).
    //*************************************************************************
    bool empty() const
    {
      return get_root() == ETL_NULLPTR;
    }

    //*************************************************************************
    /// Returns the number of elements.
    /// Complexity: O(N).
    ///
    /// Use `empty` method (with O(1) complexity) if you don't really need its exact size.
    //*************************************************************************
    size_t size() const
    {
      size_t result = 0;

#if ETL_USING_CPP11
      auto counter = [&result](const link_type& link)
      {
        if (!link.is_origin())
        {
          result++;
        }
      };
#else
      struct
      {
        size_t& result_ref;
        void operator()(const link_type& link)
        {
          if (!link.is_origin())
          {
            result_ref++;
          }
        }
      } counter = {result};
#endif

      visit_in_order_impl(&origin, false, counter);
      return result;
    }

    //*************************************************************************
    /// Unlinks all current items, leaving this tree in the empty state.
    /// Complexity: O(N).
    /// Operation invalidates all existing iterator.
    ///
    /// Note that the same "clear all" effect could be achieved by using `erase`
    /// method for each item, but b/c of intermediate tree rebalancing
    /// complexity will be higher - O(N*log(N)).
    //*************************************************************************
    void clear()
    {
#if ETL_USING_CPP11
      auto unlinker = [](link_type& link)
      {
        link.clear();
        link.etl_bf = 0;
      };
#else
      struct
      {
        void operator()(link_type& link) const
        {
          link.clear();
          link.etl_bf = 0;
        }
      } unlinker;
#endif

      // No need to balance b/c everything will be unlinked.
      // Note that "post order" visitation is important -
      // it ensures that once a link is passed to the "visitor" lambda,
      // traversal won't use pointer to this link anymore,
      // so we could efficiently clear the link.
      visit_post_order_impl(&origin, false, unlinker);
    }

  protected:
    //*************************************************************************
    /// Default constructor.
    //*************************************************************************
    intrusive_avl_tree_base()
    {
    }

#if ETL_USING_CPP11
    //*************************************************************************
    /// Move constructor.
    /// Complexity: O(1).
    //*************************************************************************
    intrusive_avl_tree_base(intrusive_avl_tree_base&&) ETL_NOEXCEPT = default;
#endif

    //*************************************************************************
    /// Destructor.
    /// Complexity: O(N).
    //*************************************************************************
    ~intrusive_avl_tree_base()
    {
      // It's important to clear, so that none of the former (but still alive) items
      // stay linked to this tree (neither directly at the root item,
      // nor transitively via "parent" links from leaf items up to the origin).
      clear();
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

      while (curr->is_right_child())
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
        return ETL_NULLPTR;
      }

      if (TLink* const next = curr->get_child(false))
      {
        return find_extremum_impl(next, true);
      }

      while (curr->is_left_child())
      {
        curr = curr->get_parent();
      }
      return curr->is_origin() ? curr : curr->get_parent();
    }

    template <typename TLink, typename Visitor>
    static void visit_in_order_impl(TLink* curr, const bool is_reverse, Visitor visitor)
    {
      TLink* prev = ETL_NULLPTR;
      while (curr != ETL_NULLPTR)
      {
        TLink* next = curr->get_parent();
        if (prev == next)
        {
          if (TLink* const child1 = curr->get_child(is_reverse))
          {
            next = child1;
          }
          else
          {
            visitor(*curr);

            if (TLink* const child2 = curr->get_child(!is_reverse))
            {
              next = child2;
            }
          }
        }
        else if (prev == curr->get_child(is_reverse))
        {
          visitor(*curr);

          if (TLink* const child2 = curr->get_child(!is_reverse))
          {
            next = child2;
          }
        }

        prev = etl::exchange(curr, next);
      }
    }

    template <typename TLink, typename Visitor>
    static void visit_post_order_impl(TLink* curr, const bool is_reverse, Visitor visitor)
    {
      TLink* prev = ETL_NULLPTR;
      while (curr != ETL_NULLPTR)
      {
        TLink* next = curr->get_parent();
        if (prev == next)
        {
          if (TLink* const child1 = curr->get_child(is_reverse))
          {
            next = child1;
          }
          else if (TLink* const child2 = curr->get_child(!is_reverse))
          {
            next = child2;
          }
          else
          {
            visitor(*curr);
          }
        }
        else if (prev == curr->get_child(is_reverse))
        {
          if (TLink* const child2 = curr->get_child(!is_reverse))
          {
            next = child2;
          }
          else
          {
            visitor(*curr);
          }
        }
        else
        {
          visitor(*curr);
        }

        prev = etl::exchange(curr, next);
      }
    }

    static int8_t get_balance_factor_impl(const link_type* const curr)
    {
      return (curr != ETL_NULLPTR) ? curr->etl_bf : 0;
    }

    template <typename TLink>
    static TLink* get_parent_impl(TLink* const curr)
    {
      if (ETL_NULLPTR == curr)
      {
        return ETL_NULLPTR;
      }
      TLink* const parent = curr->get_parent();
      if ((ETL_NULLPTR == parent) || parent->is_origin())
      {
        return ETL_NULLPTR;
      }
      return parent;
    }

    template <typename TLink>
    static TLink* get_child_impl(TLink* const curr, const bool is_right)
    {
      if (ETL_NULLPTR == curr)
      {
        return ETL_NULLPTR;
      }
      return curr->get_child(is_right);
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
        auto&      value = static_cast<TValue&>(link);
#if ETL_USING_CPP11
        find_or_insert_impl<TValue>(
          [&value, &lessComp](const TValue& other)
          {
            return lessComp(value, other) ? -1 : +1;
          },
          [&value] { return &value; });
#else
        const CompareFactory<TValue, TLessComp> compareFactory(value, lessComp);
        find_or_insert_impl<TValue>(compareFactory, compareFactory);
#endif
      }
    }

    template <typename TValue, typename TLink, typename TCompare>
    static TValue* find_impl(TLink* const root, const TCompare& comp)
    {
      // Try to find existing node.
      TLink* curr = root;
      while (ETL_NULLPTR != curr)
      {
        auto* const result = static_cast<TValue*>(curr);
        const int   cmp = comp(*result);
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
      bool       is_right = false;
      link_type* curr = get_root();
      link_type* parent = &origin;
      while (ETL_NULLPTR != curr)
      {
        auto* const result = static_cast<TValue*>(curr);
        const int   cmp = comp(*result);
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
      link_type* const result_link = static_cast<link_type*>(result);
      if ((ETL_NULLPTR == result) || (ETL_NULLPTR == result_link))
      {
        // Failed (or rejected)! The tree was not modified.
        return etl::make_pair(ETL_NULLPTR, false);
      }

      ETL_ASSERT(!result_link->is_linked(), ETL_ERROR(intrusive_avl_tree_value_is_already_linked));

      // Link the new node.
      parent->link_child(result_link, is_right);

      retrace_on_insert(result_link);

      // Successfully linked, so the tree was modified.
      return etl::make_pair(result, true);
    }

    static void erase_impl(link_type* const z_link)
    {
      link_type* parent = z_link->get_parent();
      bool is_right = z_link->is_right_child();

      if (!z_link->has_left())
      {
        parent->link_child(z_link->get_right(), is_right);
      }
      else if (!z_link->has_right())
      {
        parent->link_child(z_link->get_left(), is_right);
      }
      else
      {
        link_type* const y_link = next_in_order_impl(z_link);
        link_type* const y_link_parent = y_link->get_parent();
        y_link->etl_bf = z_link->etl_bf;
        if (z_link != y_link_parent)
        {
          y_link_parent->link_child(y_link->get_right(), y_link->is_right_child());
          link_type* const z_right = z_link->get_right();
          y_link->set_right(z_right);
          z_right->set_parent(y_link);
          parent->link_child(y_link, is_right);

          is_right = false;
          parent = y_link_parent;
        }
        else
        {
          parent->link_child(y_link, is_right);

          is_right = true;
          parent = y_link;
        }
        link_type* const z_left = z_link->get_left();
        y_link->set_left(z_left);
        z_left->set_parent(y_link);
      }

      z_link->clear();
      z_link->etl_bf = 0;

      retrace_on_erase(parent, is_right);
    }

  private:
    link_type origin;  ///< This is the origin link which left child points to the root link of the tree.

#if ETL_USING_CPP11
#else
    // Disable copy construction and assignment.
    intrusive_avl_tree_base(const intrusive_avl_tree_base&);
    intrusive_avl_tree_base& operator=(const intrusive_avl_tree_base& rhs);

    template <typename TValue, typename TLessComp>
    struct CompareFactory
    {
      TValue&          value;
      const TLessComp& lessComp;

      CompareFactory(TValue& valueRef, const TLessComp& lessCompRef)
        : value(valueRef)
        , lessComp(lessCompRef)
      {
      }

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
#endif

    static void retrace_on_insert(link_type* curr)
    {
      link_type* parent = curr->get_parent();
      while (!parent->is_origin())
      {
        const bool is_right = curr->is_right_child();
        curr = parent->adjust_balance(is_right);
        parent = curr->get_parent();
        if (curr->etl_bf == 0)
        {
          break;
        }
      }

      if (parent->is_origin())
      {
        parent->set_left(curr);
      }
    }

    static void retrace_on_erase(link_type* parent, bool is_right)
    {
      while (!parent->is_origin())
      {
        link_type* const curr = parent->adjust_balance(!is_right);
        parent = curr->get_parent();
        if ((curr->etl_bf != 0) || parent->is_origin())
        {
          if (parent->is_origin())
          {
            parent->set_left(curr);
          }
          break;
        }
        is_right = curr->is_right_child();
      }
    }

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
#if ETL_USING_CPP11
    typedef value_type&&      rvalue_reference;
#endif

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

      //*************************************************************************
      /// Increments iterator to point to the next ("greater") item.
      /// Complexity: amortized O(1), worst-case O(log(N)).
      //*************************************************************************
      iterator& operator++()
      {
        p_value = base::next_in_order_impl(p_value);
        return *this;
      }

      //*************************************************************************
      /// Increments iterator to point to the next ("greater") item.
      /// Complexity: amortized O(1), worst-case O(log(N)).
      /// Returns original iterator (before the increment).
      //*************************************************************************
      iterator operator++(int)
      {
        iterator temp(*this);
        p_value = base::next_in_order_impl(p_value);
        return temp;
      }

      //*************************************************************************
      /// Decrements iterator to point to the previous ("smaller") item.
      /// Complexity: amortized O(1), worst-case O(log(N)).
      //*************************************************************************
      iterator& operator--()
      {
        p_value = base::prev_in_order_impl(p_value);
        return *this;
      }

      //*************************************************************************
      /// Decrements iterator to point to the previous ("smaller") item.
      /// Complexity: amortized O(1), worst-case O(log(N)).
      /// Returns original iterator (before the decrement).
      //*************************************************************************
      iterator operator--(int)
      {
        iterator temp(*this);
        p_value = base::prev_in_order_impl(p_value);
        return temp;
      }

      iterator& operator=(const iterator& other)
      {
        if (this != etl::addressof(other))
        {
          p_value = other.p_value;
        }
        return *this;
      }

      reference operator*() const
      {
#include "private/diagnostic_null_dereference_push.h"
        return *static_cast<pointer>(p_value);
#include "private/diagnostic_pop.h"
      }

      pointer operator&() const
      {
        return static_cast<pointer>(p_value);
      }

      pointer operator->() const
      {
        return static_cast<pointer>(p_value);
      }

      friend bool operator==(const iterator& lhs, const iterator& rhs)
      {
        return lhs.p_value == rhs.p_value;
      }

      friend bool operator!=(const iterator& lhs, const iterator& rhs)
      {
        return !(lhs == rhs);
      }

      explicit operator bool() const
      {
        return has_value();
      }

      bool has_value() const
      {
        return p_value != ETL_NULLPTR;
      }

      //*************************************************************************
      /// Gets balance factor of tree node.
      /// Complexity: O(1).
      /// Normally is not needed unless advanced traversal is required.
      /// Result: -1, 0 or +1 depending on children tree height difference.
      //*************************************************************************
      int8_t get_balance_factor() const
      {
        return base::get_balance_factor_impl(p_value);
      }

      //*************************************************************************
      /// Gets parent node.
      /// Complexity: O(1).
      /// Normally is not needed unless advanced traversal is required.
      /// Result iterator will be valueless (`has_value() == false`) if there is no parent.
      //*************************************************************************
      iterator get_parent() const
      {
        return iterator(base::get_parent_impl(p_value));
      }

      //*************************************************************************
      /// Gets a child node.
      /// Complexity: O(1).
      /// Normally is not needed unless advanced traversal is required.
      /// Result iterator will be valueless (`has_value() == false`) if there is no such child.
      //*************************************************************************
      iterator get_child(const bool is_right) const
      {
        return iterator(base::get_child_impl(p_value, is_right));
      }

    private:
      explicit iterator(link_type* value)
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

      const_iterator(const typename intrusive_avl_tree::iterator& other)
        : p_value(other.p_value)
      {
      }

      const_iterator(const const_iterator& other)
        : p_value(other.p_value)
      {
      }

      //*************************************************************************
      /// Increments const iterator to point to the next ("greater") item.
      /// Complexity: amortized O(1), worst-case O(log(N)).
      //*************************************************************************
      const_iterator& operator++()
      {
        p_value = base::next_in_order_impl(p_value);
        return *this;
      }

      //*************************************************************************
      /// Increments const iterator to point to the next ("greater") item.
      /// Complexity: amortized O(1), worst-case O(log(N)).
      /// Returns original iterator (before the increment).
      const_iterator operator++(int)
      {
        const_iterator temp(*this);
        p_value = base::next_in_order_impl(p_value);
        return temp;
      }

      //*************************************************************************
      /// Decrements const iterator to point to the previous ("smaller") item.
      /// Complexity: amortized O(1), worst-case O(log(N)).
      //*************************************************************************
      const_iterator& operator--()
      {
        p_value = base::prev_in_order_impl(p_value);
        return *this;
      }

      //*************************************************************************
      /// Decrements const iterator to point to the previous ("smaller") item.
      /// Complexity: amortized O(1), worst-case O(log(N)).
      /// Returns original iterator (before the decrement).
      //*************************************************************************
      const_iterator operator--(int)
      {
        const_iterator temp(*this);
        p_value = base::prev_in_order_impl(p_value);
        return temp;
      }

      const_iterator& operator=(const const_iterator& other)
      {
        if (this != etl::addressof(other))
        {
          p_value = other.p_value;
        }
        return *this;
      }

      const_reference operator*() const
      {
        return *static_cast<const_pointer>(p_value);
      }

      const_pointer operator&() const
      {
        return static_cast<const_pointer>(p_value);
      }

      const_pointer operator->() const
      {
        return static_cast<const_pointer>(p_value);
      }

      friend bool operator==(const const_iterator& lhs, const const_iterator& rhs)
      {
        return lhs.p_value == rhs.p_value;
      }

      friend bool operator!=(const const_iterator& lhs, const const_iterator& rhs)
      {
        return !(lhs == rhs);
      }

      explicit operator bool() const
      {
        return has_value();
      }

      bool has_value() const
      {
        return p_value != ETL_NULLPTR;
      }

      //*************************************************************************
      /// Gets balance factor of tree node.
      /// Complexity: O(1).
      /// Normally is not needed unless advanced traversal is required.
      /// Result: -1, 0 or +1 depending on children tree height difference.
      //*************************************************************************
      int8_t get_balance_factor() const
      {
        return base::get_balance_factor_impl(p_value);
      }

      //*************************************************************************
      /// Gets parent node.
      /// Complexity: O(1).
      /// Normally is not needed unless advanced traversal is required.
      /// Result iterator will be valueless (`has_value() == false`) if there is no parent.
      //*************************************************************************
      const_iterator get_parent() const
      {
        return const_iterator(base::get_parent_impl(p_value));
      }

      //*************************************************************************
      /// Gets a child node.
      /// Complexity: O(1).
      /// Normally is not needed unless advanced traversal is required.
      /// Result iterator will be valueless (`has_value() == false`) if there is no such child.
      //*************************************************************************
      const_iterator get_child(const bool is_right) const
      {
        return const_iterator(base::get_child_impl(p_value, is_right));
      }

    private:
      explicit const_iterator(const link_type* value)
        : p_value(value)
      {
      }

      const link_type* p_value;

    };  // const_iterator

    //*************************************************************************
    /// Default constructor.
    //*************************************************************************
    intrusive_avl_tree()
      : intrusive_avl_tree_base<ID_>()
    {
    }

    //*************************************************************************
    /// Constructor from range.
    /// Complexity: O(N*log(N)).
    //*************************************************************************
    template <typename TIterator, typename TLessComp>
    intrusive_avl_tree(TIterator first, TIterator last, const TLessComp& lessComp, typename etl::enable_if<!etl::is_integral<TIterator>::value, int>::type = 0)
    {
      base::template assign_impl<value_type>(first, last, lessComp);
    }

    //*************************************************************************
    /// Destructor.
    /// Complexity: O(N).
    //*************************************************************************
    ~intrusive_avl_tree() = default;

#if ETL_USING_CPP11
    //*************************************************************************
    /// Move constructor.
    /// Complexity: O(1).
    //*************************************************************************
    intrusive_avl_tree(intrusive_avl_tree&&) ETL_NOEXCEPT = default;

    intrusive_avl_tree(const intrusive_avl_tree&) = delete;
    intrusive_avl_tree& operator=(const intrusive_avl_tree& rhs) = delete;
    intrusive_avl_tree& operator=(intrusive_avl_tree&& rhs) ETL_NOEXCEPT = delete;
#endif

    //*************************************************************************
    /// Gets the beginning of the intrusive_avl_tree.
    /// Complexity: O(log(N)).
    //*************************************************************************
    iterator begin()
    {
      return iterator(base::begin_impl(base::get_origin()));
    }

    //*************************************************************************
    /// Gets the beginning of the intrusive_avl_tree.
    /// Complexity: O(log(N)).
    //*************************************************************************
    const_iterator begin() const
    {
      return const_iterator(base::begin_impl(base::get_origin()));
    }

    //*************************************************************************
    /// Gets the beginning of the intrusive_avl_tree.
    /// Complexity: O(log(N)).
    //*************************************************************************
    const_iterator cbegin() const
    {
      return begin();
    }

    //*************************************************************************
    /// Gets the end of the intrusive_avl_tree.
    /// Complexity: O(1).
    //*************************************************************************
    iterator end()
    {
      return iterator(base::end_impl(base::get_origin()));
    }

    //*************************************************************************
    /// Gets the end of the intrusive_avl_tree.
    /// Complexity: O(1).
    //*************************************************************************
    const_iterator end() const
    {
      return const_iterator(base::end_impl(base::get_origin()));
    }

    //*************************************************************************
    /// Gets the end of the intrusive_avl_tree.
    /// Complexity: O(1).
    //*************************************************************************
    const_iterator cend() const
    {
      return end();
    }

    //*************************************************************************
    /// Gets root node (if any).
    /// Complexity: O(1).
    /// Normally is not needed unless advanced traversal is required.
    /// Result iterator will be valueless (`has_value() == false`) if tree is empty.
    //*************************************************************************
    iterator get_root()
    {
      return iterator(base::get_root());
    }

    //*************************************************************************
    /// Gets root node (if any).
    /// Complexity: O(1).
    /// Normally is not needed unless advanced traversal is required.
    /// Result iterator will be valueless (`has_value() == false`) if tree is empty.
    //*************************************************************************
    const_iterator get_root() const
    {
      return const_iterator(base::get_root());
    }

    //*************************************************************************
    /// Finds an item using given comparer.
    /// Complexity: O(log(N)) assuming O(1) for the comparer.
    ///
    /// The comparer should accept `const value_type& value` argument,
    /// and return integer result:
    /// - `> 0` if the find target is greater than the argument
    /// - `0` if the target is found
    /// - `< 0` if the target is smaller
    /// - if throws then exception is propagated
    ///
    /// Result iterator will be `end()` if there is no matching item.
    //*************************************************************************
    template <typename TCompare>
    iterator find(const TCompare& comp)
    {
      pointer ptr = base::template find_impl<value_type>(base::get_root(), comp);
      return make_iterator(ptr, end());
    }

    //*************************************************************************
    /// Finds a constant item using given comparer.
    /// Complexity: O(log(N)) assuming O(1) for the comparer.
    ///
    /// The comparer should accept `const value_type& value` argument,
    /// and return integer result:
    /// - `> 0` if the find target is greater than the argument
    /// - `0` if the target is found
    /// - `< 0` if the target is smaller
    /// - if throws then exception is propagated
    ///
    /// Result iterator will be `end()` if there is no matching item.
    //*************************************************************************
    template <typename TCompare>
    const_iterator find(const TCompare& comp) const
    {
      const_pointer ptr = base::template find_impl<const value_type>(base::get_root(), comp);
      return make_iterator(ptr, end());
    }

    //*************************************************************************
    /// Finds an existing item using given comparer, and if not found
    /// then invokes `factory` functor to insert a new item. The new item
    /// is inserted at the tree position where the search has stopped.
    /// Complexity: O(log(N)) assuming O(1) for the comparer and factory.
    /// Operation does NOT invalidate any already existing iterators.
    ///
    /// The comparer should accept `const value_type& value` argument,
    /// and return integer result:
    /// - `> 0` if the find target is greater than the argument
    /// - `0` if the target is found
    /// - `< 0` if the target is smaller
    /// - if throws then exception is propagated
    /// Hint: If result tree should contain "duplicates" then return non-zero
    /// result even for "equal" items, like:
    /// - `+1` to append a new item after existing duplicates
    /// - `-1` to prepend a new item before existing duplicates
    ///
    /// The factory functor should return address of a "new" value (castable to the `link_type*`).
    /// The returned value should not be already linked to any tree,
    /// otherwise throws `intrusive_avl_tree_value_is_already_linked`.
    /// If return address is `nullptr` then tree won't be modified,
    /// and final result iterator will be valueless.
    /// If factory throws then exception is propagated (without modifying the tree).
    ///
    /// Result contains a pair of:
    /// - iterator to found (or inserted) item (could be valueless, but never `end()`).
    /// - boolean indicating whether tree was modified (due to successful insertion).
    //*************************************************************************
    template <typename TCompare, typename TFactory>
    etl::pair<iterator, bool> find_or_insert(const TCompare& comp, const TFactory& factory)
    {
      const etl::pair<value_type*, bool> ptr_mod = base::template find_or_insert_impl<value_type>(comp, factory);
      return etl::make_pair(make_iterator(ptr_mod.first, iterator()), ptr_mod.second);
    }

    //*************************************************************************
    /// Erases the value at the specified position.
    /// Complexity: O(log(N)).
    /// Operation invalidates any existing iterator to the same item,
    /// but it does NOT affect any other iterators.
    /// Returns iterator to the next tree node (after the erased one).
    //*************************************************************************
    iterator erase(iterator position)
    {
      iterator next(position);
      ++next;

      base::erase_impl(position.p_value);

      return next;
    }

    //*************************************************************************
    /// Erases the value at the specified position.
    /// Complexity: O(log(N)).
    /// Operation invalidates any existing iterator to the same item,
    /// but it does NOT affect any other iterators.
    /// Returns iterator to the next tree node (after the erased one).
    /// Use `clear` method if you need erase all items.
    //*************************************************************************
    iterator erase(const_iterator position)
    {
      return erase(iterator(const_cast<link_type*>(position.p_value)));
    }

  private:
#if ETL_USING_CPP11
#else
    // Disable copy construction and assignment.
    intrusive_avl_tree(const intrusive_avl_tree&);
    intrusive_avl_tree& operator=(const intrusive_avl_tree& rhs);
#endif

    template <typename TIterator, typename Pointer>
    static TIterator make_iterator(Pointer const ptr, const TIterator end)
    {
      return (ptr != ETL_NULLPTR) ? TIterator(ptr) : end;
    }

  };  // intrusive_avl_tree

}  // namespace etl

#endif
