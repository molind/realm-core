/*************************************************************************
 *
 * TIGHTDB CONFIDENTIAL
 * __________________
 *
 *  [2011] - [2012] TightDB Inc
 *  All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of TightDB Incorporated and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to TightDB Incorporated
 * and its suppliers and may be covered by U.S. and Foreign Patents,
 * patents in process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from TightDB Incorporated.
 *
 **************************************************************************/
#ifndef TIGHTDB_ARRAY_BASIC_HPP
#define TIGHTDB_ARRAY_BASIC_HPP

#include <tightdb/array.hpp>

namespace tightdb {

/// A BasicArray can currently only be used for simple unstructured
/// types like float, double.
template<typename T> class BasicArray: public Array {
public:
    explicit BasicArray(ArrayParent* = 0, std::size_t ndx_in_parent = 0,
                        Allocator& = Allocator::get_default());
    BasicArray(ref_type ref, ArrayParent*, std::size_t ndx_in_parent,
               Allocator& = Allocator::get_default()) TIGHTDB_NOEXCEPT;
    explicit BasicArray(no_prealloc_tag) TIGHTDB_NOEXCEPT;

    T get(size_t ndx) const TIGHTDB_NOEXCEPT;
    void add(T value);
    void set(size_t ndx, T value);
    void insert(size_t ndx, T value);
    void erase(size_t ndx);
    void clear();

    size_t Find(T target, size_t start, size_t end) const;
    size_t find_first(T value, size_t start=0 , size_t end=-1) const;
    void find_all(Array& result, T value, size_t add_offset = 0, size_t start = 0, size_t end = -1);

    size_t count(T value, size_t start=0, size_t end=-1) const;
    // Unused: double sum(size_t start=0, size_t end=-1) const;
    bool maximum(T& result, size_t start=0, size_t end=-1) const;
    bool minimum(T& result, size_t start=0, size_t end=-1) const;

    /// Compare two arrays for equality.
    bool Compare(const BasicArray<T>&) const;

    static T column_get(const Array* root, std::size_t ndx) TIGHTDB_NOEXCEPT;

private:
    virtual size_t CalcByteLen(size_t count, size_t width) const;
    virtual size_t CalcItemCount(size_t bytes, size_t width) const TIGHTDB_NOEXCEPT;
    virtual WidthType GetWidthType() const { return wtype_Multiply; }

    template<bool find_max> bool minmax(T& result, size_t start, size_t end) const;
    static size_t create_empty_basic_array(Allocator& alloc);
};


// Class typedefs for BasicArray's: ArrayFloat and ArrayDouble
typedef BasicArray<float> ArrayFloat;
typedef BasicArray<double> ArrayDouble;

} // namespace tightdb

#include <tightdb/array_basic_tpl.hpp>

#endif // TIGHTDB_ARRAY_BASIC_HPP
