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
  /// \ingroup intrusive_avl_tree
  /// Base for intrusive AVL tree. Stores elements derived from 'intrusive_avl_tree_base<ID>::link'.
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
    struct link : private etl::tree_link<ID_>
    {
      link()
        : etl::tree_link<ID_>()
        , etl_bf(0)
      {
      }

    private:
      typedef etl::tree_link<ID_> base;
      friend class intrusive_avl_tree_base;

      link* get_child(const bool is_right)
      {
        return static_cast<link*>(is_right ? base::etl_right : base::etl_left);
      }

      const link* get_child(const bool is_right) const
      {
        return static_cast<const link*>(is_right ? base::etl_right : base::etl_left);
      }

      base*& get_child_ref(const bool is_right)
      {
        return is_right ? base::etl_right : base::etl_left;
      }

      int8_t etl_bf; ///< Stores -1, 0 or +1 balancing factor.

    };  // link

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

    link* get_root()
    {
      return static_cast<link*>(origin.etl_left);
    }

    const link* get_root() const
    {
      return static_cast<const link*>(origin.etl_left);
    }

    template <typename Pointer, typename Link, typename Comparator>
    static Pointer find_impl(Link* const root, const Comparator& comparator)
    {
      // Try to find existing node.
      Link* curr = root;
      while (ETL_NULLPTR != curr)
      {
        Pointer const result = static_cast<Pointer>(curr);
        const int cmp = comparator(*result);
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

    template <typename Pointer, typename Comparator, typename Factory>
    etl::pair<Pointer, bool> find_or_create_impl(const Comparator& comparator, const Factory& factory)
    {
      // Try to find existing node.
      bool is_right = false;
      link* curr = get_root();
      link* parent = &origin;
      while (ETL_NULLPTR != curr)
      {
        Pointer const result = static_cast<Pointer>(curr);
        const int cmp = comparator(*result);
        if (0 == cmp)
        {
          // Found! Tree was not modified.
          return etl::make_pair(result, false);
        }

        parent = curr;
        curr = curr->get_child(cmp > 0);
      }

      // Try to instantiate new node.
      const Pointer result = factory();
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
      result->etl_parent = parent;

      // TODO: Rebalance the tree.

      // Successfully linked, so the tree was modified.
      return etl::make_pair(result, true);
    }

  private:
    link origin; ///< This is the origin link which left child points to the root link of the tree.
    size_t current_size; ///< Counts the number of elements in the tree.

  };  // intrusive_avl_tree_base

  //***************************************************************************
  /// \ingroup intrusive_avl_tree
  /// An intrusive AVL tree. Stores elements derived from 'intrusive_avl_tree<ID>::link'.
  /// \warning This tree cannot be used for concurrent access from multiple threads.
  /// \tparam TValue The type of value that the tree holds.
  /// \tparam ID_ The link ID that the value is derived from.
  //***************************************************************************
  template <typename TValue, size_t ID_ = 0>
  class intrusive_avl_tree : public etl::intrusive_avl_tree_base<ID_>
  {
    typedef etl::intrusive_avl_tree_base<ID_> base;

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
      : intrusive_avl_tree_base<ID_>()
    {
    }

    template <typename Comparator>
    pointer find(const Comparator& comparator)
    {
      return base::template find_impl<pointer>(base::get_root(), comparator);
    }

    template <typename Comparator>
    const_pointer find(const Comparator& comparator) const
    {
      return base::template find_impl<const_pointer>(base::get_root(), comparator);
    }

    template <typename Comparator, typename Factory>
    etl::pair<pointer, bool> find_or_create(const Comparator& comparator, const Factory& factory)
    {
      return base::template find_or_create_impl<pointer>(comparator, factory);
    }

  private:

    // Disable copy construction and assignment.
    intrusive_avl_tree(const intrusive_avl_tree&);
    intrusive_avl_tree& operator = (const intrusive_avl_tree& rhs);

  };  // intrusive_avl_tree

}  // namespace etl

#endif
