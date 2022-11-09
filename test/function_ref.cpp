
//          Copyright Michael Florian Hava.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <catch.hpp>
#include <function_ref.hpp>

static_assert(sizeof(p2548::function_ref<int()>) == sizeof(void *) * 2);
static_assert(sizeof(p2548::function_ref<int(int)>) == sizeof(void *) * 2);
static_assert(sizeof(p2548::function_ref<int() noexcept>) == sizeof(void *) * 2);
static_assert(sizeof(p2548::function_ref<int(int) noexcept>) == sizeof(void *) * 2);

namespace {
	using free_throwing = p2548::function_ref<int()>;
	using free_noexcept = p2548::function_ref<int() noexcept>;
	using const_free_throwing = p2548::function_ref<int() const>;
	using const_free_noexcept = p2548::function_ref<int() const noexcept>;

	int func1()          { return 0; }
	int func2() noexcept { return 1; }

	class non_func {
		int val;
	public:
		non_func(int val) noexcept : val{val} {}
		auto do_() const -> int { return val; }
	} nf{0};

	int foo(non_func * nf) { return nf->do_(); }
	int bar(const non_func * nf) { return nf->do_(); }
}

static_assert(std::is_same_v<decltype(p2548::function_ref{func1}), free_throwing>);
static_assert(std::is_same_v<decltype(p2548::function_ref{&func1}), free_throwing>);
static_assert(std::is_same_v<decltype(p2548::function_ref{func2}), free_noexcept>);
static_assert(std::is_same_v<decltype(p2548::function_ref{&func2}), free_noexcept>);

TEST_CASE("function_ref function", "[function_ref]") {
	free_throwing ref1{func1};
	REQUIRE(ref1() == 0);
	const_free_throwing cref1{func1};
	REQUIRE(cref1() == 0);
	free_noexcept ref2{func2};
	REQUIRE(ref2() == 1);
	const_free_noexcept cref2{func2};
	REQUIRE(cref2() == 1);
}

TEST_CASE("function_ref function ptr", "[function_ref]") {
	free_throwing ref1{&func1};
	REQUIRE(ref1() == 0);
	const_free_throwing cref1{&func1};
	REQUIRE(cref1() == 0);
	free_noexcept ref2{&func2};
	REQUIRE(ref2() == 1);
	const_free_noexcept cref2{&func2};
	REQUIRE(cref2() == 1);
}

TEST_CASE("function_ref functor", "[function_ref]") {
	struct { auto operator()() const          -> int { return 0; } } func1;
	struct { auto operator()() const noexcept -> int { return 1; } } func2;
	struct { auto operator()()                -> int { return 2; } } func3;
	struct { auto operator()()       noexcept -> int { return 3; } } func4;

	free_throwing ref1{func1};
	REQUIRE(ref1() == 0);
	const_free_throwing cref1{func1};
	REQUIRE(cref1() == 0);
	free_noexcept ref2{func2};
	REQUIRE(ref2() == 1);
	const_free_noexcept cref2{func2};
	REQUIRE(cref2() == 1);

	free_throwing ref3{func3};
	REQUIRE(ref3() == 2);
	static_assert(!std::is_constructible_v<const_free_throwing, decltype(func3)>);
	free_noexcept ref4{func4};
	REQUIRE(ref4() == 3);
	static_assert(!std::is_constructible_v<const_free_noexcept, decltype(func4)>);
}
