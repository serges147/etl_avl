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
#include "utility.h"

#include <stddef.h>

namespace etl
{
  //***************************************************************************
  ///\ingroup intrusive_avl_tree
  /// Base for intrusive AVL tree. Stores elements derived any ETL type that supports an 'etl_next' pointer member.
  /// \tparam TLink  The link type that the value is derived from.
  //***************************************************************************
  template <typename TLink>
  class intrusive_avl_tree_base
  {
  public:

    // Node typedef.
    typedef TLink link_type;

    //*************************************************************************
    /// Checks if the tree is in the empty state.
    //*************************************************************************
    bool empty() const
    {
      return get_root() == ETL_NULLPTR;
    }

    //*************************************************************************
    /// Returns the number of elements.
    //*************************************************************************
    size_t size() const
    {
      return current_size;
    }

  protected:

    //*************************************************************************
    /// Constructor
    //*************************************************************************
    intrusive_avl_tree_base()
      : current_size(0)
    {
    }

    //*************************************************************************
    /// Destructor
    //*************************************************************************
    ~intrusive_avl_tree_base()
    {
    }

    link_type* get_origin()
    {
      return &origin;
    }

    link_type* get_root()
    {
      return origin.etl_left;
    }

    const link_type* get_root() const
    {
      return origin.etl_left;
    }

    link_type*& get_root_ref()
    {
      return origin.etl_left;
    }

    link_type origin; ///< This is the origin link which left child points to the root link of the tree.
    size_t current_size; ///< Counts the number of elements in the tree.

  };  // intrusive_avl_tree_base

  //***************************************************************************
  ///\ingroup intrusive_avl_tree
  /// An intrusive AVL tree. Stores elements derived from any type that supports an 'etl_next' pointer member.
  /// \warning This tree cannot be used for concurrent access from multiple threads.
  /// \tparam TValue The type of value that the tree holds.
  /// \tparam TLink  The link type that the value is derived from.
  //***************************************************************************
  template <typename TValue, typename TLink>
  class intrusive_avl_tree : public etl::intrusive_avl_tree_base<TLink>
  {
    typedef etl::intrusive_avl_tree_base<TLink> base;

  public:

    // STL style typedefs.
    typedef TValue            value_type;
    typedef value_type*       pointer;
    typedef const value_type* const_pointer;
    typedef value_type&       reference;
    typedef const value_type& const_reference;
    typedef size_t            size_type;

    //*************************************************************************
    /// Constructor
    //*************************************************************************
    intrusive_avl_tree()
      : intrusive_avl_tree_base<TLink>()
    {
    }

    template <typename Comparator>
    pointer find(const Comparator& comparator)
    {
      return find_impl<pointer>(base::get_root(), comparator);
    }

    template <typename Comparator>
    const_pointer find(const Comparator& comparator) const
    {
      return find_impl<const_pointer>(base::get_root(), comparator);
    }

    template <typename Comparator, typename Factory>
    etl::pair<pointer, bool> find_or_create(const Comparator& comparator, const Factory& factory)
    {
      return find_or_create_impl(comparator, factory);
    }

  private:

    // Disable copy construction and assignment.
    intrusive_avl_tree(const intrusive_avl_tree&);
    intrusive_avl_tree& operator = (const intrusive_avl_tree& rhs);

    template <typename Pointer, typename Link, typename Comparator>
    static Pointer find_impl(Link* const root, const Comparator& comparator)
    {
      // Try to find existing node.
      Link* curr = root;
      while (ETL_NULLPTR != curr)
      {
        auto* const result = static_cast<Pointer>(curr);
        const int cmp = comparator(*result);
        if (0 == cmp)
        {
          // Found!
          return result;
        }
        const bool isRight = cmp > 0;
        curr = isRight ? curr->etl_right : curr->etl_left;
      }

      // Not found.
      return ETL_NULLPTR;
    }

    template <typename Comparator, typename Factory>
    etl::pair<pointer, bool> find_or_create_impl(const Comparator& comparator, const Factory& factory)
    {
      // Try to find existing node.
      bool isRight = false;
      TLink* curr = base::get_root();
      TLink* parent = base::get_origin();
      while (ETL_NULLPTR != curr)
      {
        auto* const result = static_cast<pointer>(curr);
        const int cmp = comparator(*result);
        if (0 == cmp)
        {
          // Found! Tree was not modified.
          return etl::make_pair(result, false);
        }

        parent = curr;
        isRight = cmp > 0;
        curr = isRight ? curr->etl_right : curr->etl_left;
      }

      // Try to instantiate new node.
      auto* const result = factory();
      if (ETL_NULLPTR == result)
      {
        // Failed (or rejected)! The tree was not modified.
        return etl::make_pair(ETL_NULLPTR, false);
      }

      // Link the new node.
      if (parent != base::get_origin())
      {
        TLink*& parent_child_ref = isRight ? parent->etl_right : parent->etl_left;
        parent_child_ref = result;
        result->etl_parent = parent;
      }
      else
      {
        base::get_root_ref() = result;
        result->etl_parent = base::get_origin();
      }

      // TODO: Rebalance the tree.

      // Successfully linked, so the tree was modified.
      return etl::make_pair(result, true);
    }

  };  // intrusive_avl_tree

}  // namespace etl

#endif
