/***************************************************************************
* Copyright (c) 2017, Johan Mabille, Sylvain Corlay and Wolf Vollprecht    *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <cstddef>
#include <vector>
#include "gtest/gtest.h"
#include "xtl/xbasic_fixed_string.hpp"
#include "test_fixture_view.hpp"

namespace xf
{
    using slice_type = xt::xstepped_range<size_type>;

    // stepped_range(1, 7, 2)
    inline slice_type make_slice()
    {
        return slice_type(size_type(1), size_type(7), size_type(2));
    }

    TEST(xaxis_view, label)
    {
        auto a = make_variant_view_saxis();
        axis_view_type v = axis_view_type(a, make_slice());
        EXPECT_EQ(xtl::get<fstring>(v.label(0)), v.labels<fstring>()[0]);
    }

    TEST(xaxis_view, size)
    {
        auto a = make_variant_view_saxis();
        axis_view_type v = axis_view_type(a, make_slice());
        EXPECT_EQ(v.size(), 3);
        EXPECT_FALSE(v.empty());

        axis_view_type v2 = axis_view_type(a, slice_type(size_type(1), size_type(1), size_type(2)));
        EXPECT_EQ(v2.size(), 0u);
        EXPECT_TRUE(v2.empty());
    }

    TEST(xaxis_view, contains)
    {
        auto a = make_variant_view_saxis();
        axis_view_type v = axis_view_type(a, make_slice());

        EXPECT_TRUE(v.contains("c"));
        EXPECT_FALSE(v.contains("d"));
    }

    TEST(xaxis_view, access)
    {
        auto a = make_variant_view_saxis();
        axis_view_type v = axis_view_type(a, make_slice());

        auto vc = v["c"];
        auto vf = v["f"];
        auto vh = v["h"];

        EXPECT_EQ(vc, 1u);
        EXPECT_EQ(vf, 3u);
        EXPECT_EQ(vh, 5u);
        EXPECT_THROW(v["m"], std::out_of_range);
    }

    TEST(xaxis_view, iterator)
    {
        auto a = make_variant_view_saxis();
        axis_view_type v = axis_view_type(a, make_slice());

        auto it = v.begin();

        EXPECT_TRUE(it == v.cbegin());
        bool res = (it != v.end());
        EXPECT_TRUE(it != v.end());
        EXPECT_TRUE(it < v.end());
        EXPECT_TRUE(it <= v.end());
        EXPECT_FALSE(it > v.end());
        EXPECT_FALSE(it >= v.end());
        EXPECT_EQ(v.end(), v.cend());

        // TODO: fix xaxis_variant iterator
        EXPECT_EQ(xtl::xget<const fstring&>(it->first), "c");
        auto val = *it;
        EXPECT_EQ(xtl::xget<const fstring&>(val.first), "c");
        EXPECT_EQ(val.second, v["c"]);
        ++it;
        EXPECT_EQ(xtl::xget<const fstring&>(it->first), "f");
        EXPECT_EQ(it->second, v["f"]);
        auto tmp = it++;
        EXPECT_EQ(xtl::xget<const fstring&>(tmp->first), "f");
        EXPECT_EQ(tmp->second, v["f"]);
        EXPECT_EQ(xtl::xget<const fstring&>(it->first), "h");
        EXPECT_EQ(it->second, v["h"]);
        ++it;
        EXPECT_EQ(it, v.end());

        EXPECT_EQ(v["h"], (v.begin() + 2)->second);
        EXPECT_EQ(v["f"], (v.end() - 2)->second);
        EXPECT_EQ(3, v.end() - v.begin());

        auto f1 = v.find("c");
        EXPECT_EQ(f1->second, v["c"]);
        auto f2 = v.find("a");
        EXPECT_EQ(f2, v.cend());
    }

    TEST(xaxis_view, range)
    {
        // { "a", "c", "d", "f", "g", "h", "m", "n" }
        auto a = make_variant_view_saxis();

        auto r = range("c", "f");
        axis_view_type vr = axis_view_type(a, r.build_index_slice(a));
        auto vrit = vr.cbegin();
        EXPECT_EQ(xtl::xget<const fstring&>(vrit->first), "c");
        ++vrit;
        EXPECT_EQ(xtl::xget<const fstring&>(vrit->first), "d");
        ++vrit;
        EXPECT_EQ(xtl::xget<const fstring&>(vrit->first), "f");
        ++vrit;
        EXPECT_EQ(vrit, vr.cend());

        auto sr = range("c", "h", 2);
        axis_view_type vsr = axis_view_type(a, sr.build_index_slice(a));
        auto vsrit = vsr.cbegin();
        EXPECT_EQ(xtl::xget<const fstring&>(vsrit->first), "c");
        ++vsrit;
        EXPECT_EQ(xtl::xget<const fstring&>(vsrit->first), "f");
        ++vsrit;
        EXPECT_EQ(xtl::xget<const fstring&>(vsrit->first), "h");
        ++vsrit;
        EXPECT_EQ(vsrit, vsr.cend());
    }

    /*TEST(xaxis_view, slice_conversion)
    {
        using saxis_slice_type = xt::xaxis_index_slice<std::size_t>;
        using iaxis_slice_type = xt::xaxis_index_slice<int>;

        iaxis_slice_type is1 = xt::xall<int>(4);
        saxis_slice_type ss1(is1);
        EXPECT_EQ(ss1.size(), std::size_t(4));

        iaxis_slice_type is2 = xt::variant_range(2, 4);
        saxis_slice_type ss2(is2);
        EXPECT_EQ(ss2.size(), std::size_t(is2.size()));

        iaxis_slice_type is3 = xt::variant_range(2, 9, 2);
        saxis_slice_type ss3(is3);
        EXPECT_EQ(ss3.size(), std::size_t(is3.size()));
        EXPECT_EQ(ss3.step_size(0), std::size_t(is3.step_size(0)));

        using sextended_type = xt::xslice_extended<std::size_t>;
        using iextended_type = xt::xslice_extended<int>;

        iextended_type is4 = is3;
        sextended_type ss4 = is4;
        sextended_type res = ss3;
        EXPECT_EQ(*(ss4.get_slice()), *(res.get_slice()));

        iextended_type is5 = 2;
        sextended_type ss5 = is5;
        sextended_type res2 = std::size_t(2);
        EXPECT_EQ(*(is5.get_squeeze()), *(ss5.get_squeeze()));
    }*/

    TEST(xaxis_view, conversion)
    {
        auto a = make_variant_view_saxis();
        axis_view_type v = axis_view_type(a, make_slice());
        axis_variant vf = axis_variant(v);
        EXPECT_EQ(vf["c"], 0u);
        EXPECT_EQ(vf["f"], 1u);
        EXPECT_EQ(vf["h"], 2u);
    }
}
