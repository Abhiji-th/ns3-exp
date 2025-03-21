/*
 * Copyright (c) 2021 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef TUPLE_H
#define TUPLE_H

#include "attribute-helper.h"
#include "string.h"

#include <algorithm>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ns3
{

/**
 * @brief Stream insertion operator.
 * See https://en.cppreference.com/w/cpp/utility/apply
 *
 * Prints tuple values separated by a comma. E.g., if the tuple contains
 * v1, v2 and v3, then "v1, v2, v3" will be added to the stream.
 *
 * @tparam Args \deduced Tuple arguments
 * @param os the output stream
 * @param t the tuple
 * @returns a reference to the stream
 */
template <class... Args>
std::ostream&
operator<<(std::ostream& os, const std::tuple<Args...>& t)
{
    std::apply(
        [&os](auto&&... args) {
            std::size_t n{0};
            ((os << args << (++n != sizeof...(Args) ? ", " : "")), ...);
        },
        t);
    return os;
}

/**
 * @ingroup attributes
 * @defgroup attribute_Tuple Tuple Attribute
 * AttributeValue implementation for Tuple
 */

/**
 * @ingroup attribute_Tuple
 *
 * AttributeValue implementation for Tuple.
 *
 * Hold objects of type std::tuple<Args...>.
 * @tparam Args \explicit The list of AttributeValues to be held by this TupleValue
 *
 * @see AttributeValue
 */
template <class... Args>
class TupleValue : public AttributeValue
{
  public:
    /** Type of value stored in the TupleValue. */
    typedef std::tuple<Args...> value_type;
    /** Type returned by Get or passed in Set. */
    typedef std::tuple<std::invoke_result_t<decltype(&Args::Get), Args>...> result_type;

    TupleValue();

    /**
     * Construct this TupleValue from a std::tuple
     *
     * @param [in] value Value with which to construct.
     */
    TupleValue(const result_type& value);

    Ptr<AttributeValue> Copy() const override;
    bool DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker) override;
    std::string SerializeToString(Ptr<const AttributeChecker> checker) const override;

    /**
     * Get the stored values as a std::tuple.
     *
     * This differs from the actual value stored in the object which is
     * a tuple of Ptr<AV> where AV is a class derived from AttributeValue.
     * @return stored values as a std::tuple
     */
    result_type Get() const;
    /**
     * Set the stored values.
     *
     * @param value The stored value
     */
    void Set(const result_type& value);

    /**
     * Get the attribute values as a tuple.
     * @return the attribute values as a tuple
     */
    value_type GetValue() const;

    /**
     * Set the given variable to the values stored by this TupleValue object.
     *
     * @tparam T \deduced the type of the given variable (normally, the argument type
     *           of a set method or the type of a data member)
     * @param [out] value The stored value
     * @return true if the given variable was set
     */
    template <typename T>
    bool GetAccessor(T& value) const;

  private:
    /**
     * Set the attribute values starting from the given values.
     * Used by DeserializeFromString method.
     *
     * @tparam Is \deduced index sequence
     * @param values the given attribute values
     * @return true if the attribute values of this object were set
     */
    template <std::size_t... Is>
    bool SetValueImpl(std::index_sequence<Is...>, const std::vector<Ptr<AttributeValue>>& values);

    value_type m_value; //!< Tuple of attribute values
};

/**
 * @ingroup attribute_Tuple
 *
 * Create a TupleValue object. Enable to write code like this snippet:
 *
 * @code
 * typedef std::tuple<uint16_t, double> Tuple;
 * typedef std::tuple<UintegerValue, DoubleValue> Pack;
 *
 * TupleValue<UintegerValue, DoubleValue> t = MakeTupleValue<Pack> (Tuple {10, 1.5});
 * @endcode
 *
 * @tparam T1 \explicit A std::tuple of the AttributeValue types included in TupleValue
 * @tparam T2 \deduced A std::tuple of the type of elements stored by TupleValue
 * @param t the tuple of elements stored by TupleValue
 * @return a TupleValue object
 */
template <class T1, class T2>
auto MakeTupleValue(T2 t);

/**
 * @ingroup attribute_Tuple
 *
 * Checker for attribute values storing tuples.
 */
class TupleChecker : public AttributeChecker
{
  public:
    /**
     * Get the checkers for all tuple elements.
     *
     * @return the checkers for all tuple elements
     */
    virtual const std::vector<Ptr<const AttributeChecker>>& GetCheckers() const = 0;
};

/**
 * @ingroup attribute_Tuple
 *
 * Create a TupleChecker from AttributeCheckers associated with TupleValue elements.
 *
 * @tparam Args \explicit Attribute value types
 * @tparam Ts \deduced Attribute checker types
 * @param checkers attribute checkers
 * @return Pointer to TupleChecker instance.
 */
template <class... Args, class... Ts>
Ptr<const AttributeChecker> MakeTupleChecker(Ts... checkers);

/**
 * @ingroup attribute_Tuple
 *
 * Create an AttributeAccessor for a class data member of type tuple,
 * or a lone class get functor or set method.
 *
 * @tparam Args \explicit Attribute value types
 * @tparam T1 \deduced The type of the class data member,
 *            or the type of the class get functor or set method.
 * @param a1 The address of the data member,
 *           or the get or set method.
 * @return the AttributeAccessor
 */
template <class... Args, class T1>
Ptr<const AttributeAccessor> MakeTupleAccessor(T1 a1);

/**
 * @ingroup attribute_Tuple
 *
 * Create an AttributeAccessor using a pair of get functor
 * and set methods from a class.
 *
 * @tparam Args \explicit Attribute value types
 * @tparam T1 \deduced The type of the class data member,
 *            or the type of the class get functor or set method.
 * @tparam T2 \deduced The type of the getter class functor method.
 * @param a2 The address of the class method to set the attribute.
 * @param a1 The address of the data member, or the get or set method.
 * @return the AttributeAccessor
 */
template <class... Args, class T1, class T2>
Ptr<const AttributeAccessor> MakeTupleAccessor(T1 a1, T2 a2);

} // namespace ns3

/*****************************************************************************
 * Implementation below
 *****************************************************************************/

namespace ns3
{

template <class... Args>
TupleValue<Args...>::TupleValue()
    : m_value(std::make_tuple(Args()...))
{
}

template <class... Args>
TupleValue<Args...>::TupleValue(const result_type& value)
{
    Set(value);
}

template <class... Args>
Ptr<AttributeValue>
TupleValue<Args...>::Copy() const
{
    return Create<TupleValue<Args...>>(Get());
}

template <class... Args>
template <std::size_t... Is>
bool
TupleValue<Args...>::SetValueImpl(std::index_sequence<Is...>,
                                  const std::vector<Ptr<AttributeValue>>& values)
{
    auto valueTuple = std::make_tuple(DynamicCast<Args>(values[Is])...);

    bool ok = ((std::get<Is>(valueTuple) != nullptr) && ...);

    if (ok)
    {
        m_value = std::make_tuple(Args(*std::get<Is>(valueTuple))...);
    }
    return ok;
}

template <class... Args>
bool
TupleValue<Args...>::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    auto tupleChecker = DynamicCast<const TupleChecker>(checker);
    if (!tupleChecker)
    {
        return false;
    }

    auto count = tupleChecker->GetCheckers().size();
    if (count != sizeof...(Args))
    {
        return false;
    }

    if (value.empty() || value.front() != '{' || value.back() != '}')
    {
        return false;
    }

    value.erase(value.begin());
    value.pop_back();

    std::istringstream iss(value);
    std::vector<Ptr<AttributeValue>> values;
    std::size_t i = 0;

    for (std::string elem; std::getline(iss, elem, ',');)
    {
        // remove leading whitespaces
        std::istringstream tmp{elem};
        std::getline(tmp >> std::ws, value);

        if (i >= count)
        {
            return false;
        }
        values.push_back(tupleChecker->GetCheckers().at(i++)->CreateValidValue(StringValue(value)));
        if (!values.back())
        {
            return false;
        }
    }

    if (i != count)
    {
        return false;
    }

    return SetValueImpl(std::index_sequence_for<Args...>{}, values);
}

template <class... Args>
std::string
TupleValue<Args...>::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    std::ostringstream oss;
    oss << "{" << Get() << "}";
    return oss.str();
}

template <class... Args>
typename TupleValue<Args...>::result_type
TupleValue<Args...>::Get() const
{
    return std::apply([](Args... values) { return std::make_tuple(values.Get()...); }, m_value);
}

template <class... Args>
void
TupleValue<Args...>::Set(const typename TupleValue<Args...>::result_type& value)
{
    m_value = std::apply([](auto&&... args) { return std::make_tuple(Args(args)...); }, value);
}

template <class... Args>
typename TupleValue<Args...>::value_type
TupleValue<Args...>::GetValue() const
{
    return m_value;
}

template <class... Args>
template <typename T>
bool
TupleValue<Args...>::GetAccessor(T& value) const
{
    value = T(Get());
    return true;
}

// This internal class defines templated TupleChecker class that is instantiated
// in MakeTupleChecker. The non-templated base ns3::TupleChecker is returned in that
// function. This is the same pattern as ObjectPtrContainer.
namespace internal
{

/**
 * @ingroup attribute_Tuple
 *
 * Internal checker class templated to each AttributeChecker
 * for each entry in the tuple.
 */
template <class... Args>
class TupleChecker : public ns3::TupleChecker
{
  public:
    /**
     * Constructor.
     * @tparam Ts \deduced the type of the attribute checkers
     * @param checkers the attribute checkers for individual elements of the tuple
     */
    template <class... Ts>
    TupleChecker(Ts... checkers)
        : m_checkers{checkers...}
    {
    }

    const std::vector<Ptr<const AttributeChecker>>& GetCheckers() const override
    {
        return m_checkers;
    }

    bool Check(const AttributeValue& value) const override
    {
        const auto v = dynamic_cast<const TupleValue<Args...>*>(&value);
        if (v == nullptr)
        {
            return false;
        }
        return std::apply(
            [this](Args... values) {
                std::size_t n{0};
                return (m_checkers[n++]->Check(values) && ...);
            },
            v->GetValue());
    }

    std::string GetValueTypeName() const override
    {
        return "ns3::TupleValue";
    }

    bool HasUnderlyingTypeInformation() const override
    {
        return false;
    }

    std::string GetUnderlyingTypeInformation() const override
    {
        return "";
    }

    Ptr<AttributeValue> Create() const override
    {
        return ns3::Create<TupleValue<Args...>>();
    }

    bool Copy(const AttributeValue& source, AttributeValue& destination) const override
    {
        const auto src = dynamic_cast<const TupleValue<Args...>*>(&source);
        auto dst = dynamic_cast<TupleValue<Args...>*>(&destination);
        if (src == nullptr || dst == nullptr)
        {
            return false;
        }
        *dst = *src;
        return true;
    }

  private:
    std::vector<Ptr<const AttributeChecker>> m_checkers; //!< attribute checkers
};

/**
 * @ingroup attribute_Tuple
 *
 * Helper class defining static methods for MakeTupleChecker and MakeTupleAccessor
 * that are called when user specifies the list of AttributeValue types included
 * in a TupleValue type.
 */
template <class... Args>
struct TupleHelper
{
    /**
     * @copydoc ns3::MakeTupleChecker
     */
    template <class... Ts>
    static Ptr<const AttributeChecker> MakeTupleChecker(Ts... checkers)
    {
        return Create<internal::TupleChecker<Args...>>(checkers...);
    }

    /**
     * @copydoc ns3::MakeTupleAccessor(T1)
     */
    template <class T1>
    static Ptr<const AttributeAccessor> MakeTupleAccessor(T1 a1)
    {
        return MakeAccessorHelper<TupleValue<Args...>>(a1);
    }

    /**
     * @copydoc ns3::MakeTupleAccessor(T1,T2)
     */
    template <class T1, class T2>
    static Ptr<const AttributeAccessor> MakeTupleAccessor(T1 a1, T2 a2)
    {
        return MakeAccessorHelper<TupleValue<Args...>>(a1, a2);
    }
};

/**
 * @ingroup attribute_Tuple
 *
 * Helper class defining static methods for MakeTupleValue, MakeTupleChecker and
 * MakeTupleAccessor that are called when user provides a std::tuple of the
 * AttributeValue types included in a TupleValue type.
 * This struct is a partial specialization of struct TupleHelper.
 */
template <class... Args>
struct TupleHelper<std::tuple<Args...>>
{
    /**
     * @copydoc ns3::MakeTupleValue
     */
    static TupleValue<Args...> MakeTupleValue(const typename TupleValue<Args...>::result_type& t)
    {
        return TupleValue<Args...>(t);
    }

    /**
     * @copydoc ns3::MakeTupleChecker
     */
    template <class... Ts>
    static Ptr<const AttributeChecker> MakeTupleChecker(Ts... checkers)
    {
        return Create<internal::TupleChecker<Args...>>(checkers...);
    }

    /**
     * @copydoc ns3::MakeTupleAccessor(T1)
     */
    template <class T1>
    static Ptr<const AttributeAccessor> MakeTupleAccessor(T1 a1)
    {
        return MakeAccessorHelper<TupleValue<Args...>>(a1);
    }

    /**
     * @copydoc ns3::MakeTupleAccessor(T1,T2)
     */
    template <class T1, class T2>
    static Ptr<const AttributeAccessor> MakeTupleAccessor(T1 a1, T2 a2)
    {
        return MakeAccessorHelper<TupleValue<Args...>>(a1, a2);
    }
};

} // namespace internal

template <class T1, class T2>
auto
MakeTupleValue(T2 t)
{
    return internal::TupleHelper<T1>::MakeTupleValue(t);
}

template <class... Args, class... Ts>
Ptr<const AttributeChecker>
MakeTupleChecker(Ts... checkers)
{
    return internal::TupleHelper<Args...>::template MakeTupleChecker<Ts...>(checkers...);
}

template <class... Args, class T1>
Ptr<const AttributeAccessor>
MakeTupleAccessor(T1 a1)
{
    return internal::TupleHelper<Args...>::template MakeTupleAccessor<T1>(a1);
}

template <class... Args, class T1, class T2>
Ptr<const AttributeAccessor>
MakeTupleAccessor(T1 a1, T2 a2)
{
    return internal::TupleHelper<Args...>::template MakeTupleAccessor<T1, T2>(a1, a2);
}

} // namespace ns3

#endif // TUPLE_H
