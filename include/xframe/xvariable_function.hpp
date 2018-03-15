/***************************************************************************
* Copyright (c) 2017, Johan Mabille, Sylvain Corlay and Wolf Vollprecht    *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XFRAME_XVARIABLE_FUNCTION_HPP
#define XFRAME_XVARIABLE_FUNCTION_HPP

#include "xtensor/xoptional.hpp"

#include "xcoordinate.hpp"
#include "xselecting.hpp"
#include "xvariable_scalar.hpp"

namespace std
{
    template <class T>
    struct common_type<T, xf::xfull_coordinate>
    {
        using type = T;
    };

    template <class T>
    struct common_type<xf::xfull_coordinate, T>
        : common_type<T, xf::xfull_coordinate>
    {
    };

    template <>
    struct common_type<xf::xfull_coordinate, xf::xfull_coordinate>
    {
        using type = xf::xfull_coordinate;
    };
}

namespace xf
{
    namespace detail
    {
        template <class CT>
        struct xvariable_closure
        {
            using type = CT;
        };

        template <class CT>
        struct xvariable_closure<xt::xscalar<CT>>
        {
            using type = xvariable_scalar<CT>;
        };
    }

    template <class CT>
    using xvariable_closure_t = typename detail::xvariable_closure<CT>::type;

    /**********************
     * xvariable_function *
     **********************/

    template <class F, class R, class... CT>
    class xvariable_function : public xt::xexpression<xvariable_function<F, R, CT...>>
    {
    public:

        using self_type = xvariable_function<F, R, CT...>;
        using functor_type = std::remove_reference_t<F>;
        using data_type = xt::xoptional_function<F, R, decltype(std::declval<xvariable_closure_t<CT>>().data())...>;
        using value_type = R;
        using reference = value_type;
        using const_reference = value_type;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using size_type = xt::detail::common_size_type_t<std::decay_t<CT>...>;
        using difference_type = xt::detail::common_difference_type_t<std::decay_t<CT>...>;     

        using coordinate_type = std::common_type_t<typename std::decay_t<xvariable_closure_t<CT>>::coordinate_type...>;
        using dimension_type = std::common_type_t<typename std::decay_t<xvariable_closure_t<CT>>::dimension_type...>;
        using dimension_list = typename dimension_type::label_list;

        using expression_tag = xvariable_expression_tag;

        template <class Func, class U = std::enable_if<!std::is_base_of<Func, self_type>::value>>
        xvariable_function(Func&& f, CT... e) noexcept;

        template <class Join = DEFAULT_JOIN>
        size_type size() const noexcept;

        template <class Join = DEFAULT_JOIN>
        size_type dimension() const noexcept;

        template <class Join = DEFAULT_JOIN>
        const dimension_list& dimension_labels() const;

        template <class Join = DEFAULT_JOIN>
        const coordinate_type& coordinates() const;

        template <class Join = DEFAULT_JOIN>
        const dimension_type& dimension_mapping() const;

        template <class... Args>
        const_reference operator()(Args... args) const;
        
        template <class Join = DEFAULT_JOIN>
        xtrivial_broadcast broadcast_coordinates(coordinate_type& coords) const;
        bool broadcast_dimensions(dimension_type& dims, bool trivial_bc = false) const;

        data_type data() const noexcept;

        template <std::size_t N = dynamic()>
        using selector_type = xselector<coordinate_type, dimension_type, N>;
        template <std::size_t N = dynamic()>
        using selector_map_type = typename selector_type<N>::map_type;

        template <class Join = DEFAULT_JOIN, std::size_t N = std::numeric_limits<size_type>::max()>
        const_reference select(const selector_map_type<N>& selector) const;

        template <class Join = DEFAULT_JOIN, std::size_t N = dynamic()>
        const_reference select(selector_map_type<N>&& selector) const;

        const std::tuple<xvariable_closure_t<CT>...>& arguments() const { return m_e; }
    private:

        template <class Join>
        void compute_coordinates() const;

        template <std::size_t... I, class... Args>
        const_reference access_impl(std::index_sequence<I...>, Args... args) const;

        template <class Join, std::size_t... I>
        xtrivial_broadcast broadcast_coordinates_impl(std::index_sequence<I...>, coordinate_type& coords) const;

        template <std::size_t... I>
        data_type data_impl(std::index_sequence<I...>) const noexcept;

        template <class Join, std::size_t... I, class S>
        const_reference select_impl(std::index_sequence<I...>, S&& selector) const;

        template <std::size_t...I>
        bool merge_dimension_mapping(std::index_sequence<I...>, dimension_type& dims) const;

        std::tuple<xvariable_closure_t<CT>...> m_e;
        functor_type m_f;
        mutable coordinate_type m_coordinate;
        mutable dimension_type m_dimension_mapping;
        mutable join::join_id m_join_id;
        mutable bool m_coordinate_computed;
    };

    /*************************************
     * xvariable_function implementation *
     *************************************/

    template <class F, class R, class... CT>
    template <class Func, class>
    inline xvariable_function<F, R, CT...>::xvariable_function(Func&& f, CT... e) noexcept
        : m_e(e...),
          m_f(std::forward<Func>(f)),
          m_coordinate(),
          m_dimension_mapping(),
          m_join_id(join::inner::id()),
          m_coordinate_computed(false)
    {
    }

    template <class F, class R, class... CT>
    template <class Join>
    inline auto xvariable_function<F, R, CT...>::size() const noexcept -> size_type
    {
        const coordinate_type& coords = coordinates<Join>();
        return std::accumulate(coords.begin(), coords.end(), size_type(1),
                [](size_type init, auto&& arg) { return init * arg.second.size(); });
    }

    template <class F, class R, class... CT>
    template <class Join>
    inline auto xvariable_function<F, R, CT...>::dimension() const noexcept -> size_type
    {
        const coordinate_type& coords = coordinates<Join>();
        return coords.size();
    }
    
    template <class F, class R, class... CT>
    template <class Join>
    inline auto xvariable_function<F, R, CT...>::dimension_labels() const-> const dimension_list&
    {
        return dimension_mapping<Join>().labels();
    }

    template <class F, class R, class... CT>
    template <class Join>
    inline auto xvariable_function<F, R, CT...>::coordinates() const -> const coordinate_type&
    {
        compute_coordinates<Join>();
        return m_coordinate;
    }

    template <class F, class R, class... CT>
    template <class Join>
    inline auto xvariable_function<F, R, CT...>::dimension_mapping() const -> const dimension_type&
    {
        compute_coordinates<Join>();
        return m_dimension_mapping;
    }
    
    template <class F, class R, class... CT>
    template <class... Args>
    inline auto xvariable_function<F, R, CT...>::operator()(Args... args) const -> const_reference
    {
        return access_impl(std::make_index_sequence<sizeof...(CT)>(), static_cast<size_type>(args)...);
    }

    template <class F, class R, class... CT>
    template <class Join>
    inline xtrivial_broadcast xvariable_function<F, R, CT...>::broadcast_coordinates(coordinate_type& coords) const
    {
        return broadcast_coordinates_impl<Join>(std::make_index_sequence<sizeof...(CT)>(), coords);
    }

    namespace detail
    {
        template <class T, std::size_t I, bool C>
        struct first_non_scalar_impl
        {
            using elem_type = std::tuple_element_t<I, T>;

            static elem_type& get(const T& t) noexcept
            {
                return std::get<I>(t);
            }
        };

        template <class T, std::size_t I>
        struct first_non_scalar_impl<T, I, false>
            : first_non_scalar_impl<T, I + 1, !is_xvariable_scalar<std::tuple_element_t<I+1, T>>::value>
        {
        };

        template <class T>
        struct first_non_scalar
            : first_non_scalar_impl<T, 0, !is_xvariable_scalar<std::tuple_element_t<0, T>>::value>
        {
        };

        template <class T>
        inline auto get_first_non_scalar(const T& t) noexcept
        {
            return first_non_scalar<T>::get(t);
        }
    }

    template <class F, class R, class... CT>
    inline bool xvariable_function<F, R, CT...>::broadcast_dimensions(dimension_type& dims, bool trivial_bc) const
    {
        bool ret = true;
        if(trivial_bc)
        {
            //dims = std::get<0>(m_e).dimension_mapping();
            dims = detail::get_first_non_scalar(m_e).dimension_mapping();
        }
        else
        {
            ret = merge_dimension_mapping(std::make_index_sequence<sizeof...(CT)>(), dims);
        }
        return ret;
    }

    template <class F, class R, class... CT>
    inline auto xvariable_function<F, R, CT...>::data() const noexcept -> data_type
    {
        return data_impl(std::make_index_sequence<sizeof...(CT)>());
    }

    template <class F, class R, class... CT>
    template <class Join, std::size_t N>
    inline auto xvariable_function<F, R, CT...>::select(const selector_map_type<N>& selector) const -> const_reference
    {
        return select_impl<Join>(std::make_index_sequence<sizeof...(CT)>(), selector);
    }

    template <class F, class R, class... CT>
    template <class Join, std::size_t N>
    inline auto xvariable_function<F, R, CT...>::select(selector_map_type<N>&& selector) const -> const_reference
    {
        return select_impl<Join>(std::make_index_sequence<sizeof...(CT)>(), std::move(selector));
    }

    template <class F, class R, class... CT>
    template <class Join>
    inline void xvariable_function<F, R, CT...>::compute_coordinates() const
    {
        if(!m_coordinate_computed || m_join_id != Join::id())
        {
            m_coordinate.clear();
            auto res = broadcast_coordinates<Join>(m_coordinate);
            broadcast_dimensions(m_dimension_mapping, res.m_xtensor_trivial);
            m_coordinate_computed = true;
            m_join_id = Join::id();
        }
    }

    template <class F, class R, class... CT>
    template <std::size_t... I, class... Args>
    inline auto xvariable_function<F, R, CT...>::access_impl(std::index_sequence<I...>, Args... args) const -> const_reference
    {
        return m_f(xt::detail::get_element(std::get<I>(m_e), args...)...);
    }
    
    template <class F, class R, class... CT>
    template <class Join, std::size_t... I>
    inline xtrivial_broadcast
    xvariable_function<F, R, CT...>::broadcast_coordinates_impl(std::index_sequence<I...>, coordinate_type& coords) const
    {
        return xf::broadcast_coordinates<Join>(coords, std::get<I>(m_e).coordinates()...);
    }

    template <class F, class R, class... CT>
    template <std::size_t... I>
    inline auto xvariable_function<F, R, CT...>::data_impl(std::index_sequence<I...>) const noexcept -> data_type
    {
        return data_type(m_f, std::get<I>(m_e).data()...);
    }
    
    template <class F, class R, class... CT>
    template <class Join, std::size_t... I, class S>
    inline auto xvariable_function<F, R, CT...>::select_impl(std::index_sequence<I...>, S&& selector) const -> const_reference
    {
        return m_f(std::get<I>(m_e).template select<Join>(selector)...);
    }

    template <class F, class R, class... CT>
    template <std::size_t...I>
    bool xvariable_function<F, R, CT...>::merge_dimension_mapping(std::index_sequence<I...>, dimension_type& dims) const
    {
        return xf::broadcast_dimensions(dims, std::get<I>(m_e).dimension_mapping()...);
    }
}

#endif
