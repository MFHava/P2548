
//          Copyright Michael Florian Hava.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <catch.hpp>
#include <functional>
#include <function_ref.hpp>
#include <copyable_function.hpp>


using Signature    = void(void);


TEST_CASE("function", "[conversion] [function]") {
	p2548::function_ref<Signature>{std::function<Signature>{}};
	p2548::copyable_function<Signature>{std::function<Signature>{}};
	p2548::move_only_function<Signature>{std::function<Signature>{}};
}

TEST_CASE("move_only_function", "[conversion] [move_only_function]") {
	p2548::function_ref<Signature>{p2548::move_only_function<Signature>{}};
	//fails copy_constructible-mandate: p2548::copyable_function<Signature>{p2548::move_only_function<Signature>{}};
	//fails copy_constructible-mandate: std::function<Signature>{p2548::move_only_function<Signature>{}};
}

TEST_CASE("copyable_function", "[conversion] [copyable_function]") {
	p2548::function_ref<Signature>{p2548::copyable_function<Signature>{}};
	p2548::move_only_function<Signature>{p2548::copyable_function<Signature>{}};
	std::function<Signature>{p2548::copyable_function<Signature>{}};

}

TEST_CASE("function_ref", "[conversion] [function_ref]") {
	auto func = []{};

	std::function<Signature>{p2548::function_ref<Signature>{func}};
	p2548::move_only_function<Signature>{p2548::function_ref<Signature>{func}};
	p2548::copyable_function<Signature>{p2548::function_ref<Signature>{func}};
}
