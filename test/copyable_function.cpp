
//          Copyright Michael Florian Hava.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <catch.hpp>
#include <copyable_function.hpp>

static_assert(sizeof(p2548::copyable_function<void()                  >) == 4 * sizeof(void *));
static_assert(sizeof(p2548::copyable_function<void()       &          >) == 4 * sizeof(void *));
static_assert(sizeof(p2548::copyable_function<void()       &&         >) == 4 * sizeof(void *));
static_assert(sizeof(p2548::copyable_function<void() const            >) == 4 * sizeof(void *));
static_assert(sizeof(p2548::copyable_function<void() const &          >) == 4 * sizeof(void *));
static_assert(sizeof(p2548::copyable_function<void() const &&         >) == 4 * sizeof(void *));
static_assert(sizeof(p2548::copyable_function<void()          noexcept>) == 4 * sizeof(void *));
static_assert(sizeof(p2548::copyable_function<void()       &  noexcept>) == 4 * sizeof(void *));
static_assert(sizeof(p2548::copyable_function<void()       && noexcept>) == 4 * sizeof(void *));
static_assert(sizeof(p2548::copyable_function<void() const    noexcept>) == 4 * sizeof(void *));
static_assert(sizeof(p2548::copyable_function<void() const &  noexcept>) == 4 * sizeof(void *));
static_assert(sizeof(p2548::copyable_function<void() const && noexcept>) == 4 * sizeof(void *));

namespace {
	using free_throwing = p2548::copyable_function<int()>;
	using free_noexcept = p2548::copyable_function<int() noexcept>;
	using const_free_throwing = p2548::copyable_function<int() const>;
	using const_free_noexcept = p2548::copyable_function<int() const noexcept>;

	int func1()          { return 0; }
	int func2() noexcept { return 1; }
}

TEST_CASE("copyable_function nullptr", "[copyable_function]") {
	p2548::copyable_function<int() noexcept> fun{nullptr};
	REQUIRE(!static_cast<bool>(fun));
	REQUIRE(!fun);
	REQUIRE(fun == nullptr);

	fun = func2;
	REQUIRE(static_cast<bool>(fun));
	REQUIRE(!!fun);
	REQUIRE(fun != nullptr);

	fun = nullptr;
	REQUIRE(!static_cast<bool>(fun));
	REQUIRE(!fun);
	REQUIRE(fun == nullptr);

	fun = func2;
	REQUIRE(static_cast<bool>(fun));
	REQUIRE(!!fun);
	REQUIRE(fun != nullptr);

	fun = static_cast<decltype(&func2)>(nullptr);
	REQUIRE(!static_cast<bool>(fun));
	REQUIRE(!fun);
	REQUIRE(fun == nullptr);
}

TEST_CASE("copyable_function inplace", "[copyable_function]") {
	struct functor {
		functor(int x) : x{x} {}

		auto operator()(int y) noexcept -> int { return x + y; }
	private:
		int x;
	};

	p2548::copyable_function<int(int) noexcept> func{std::in_place_type<functor>, 10};
	REQUIRE(func(1) == 11);

	//TODO: initializer_list support
}

TEST_CASE("copyable_function free copyable_function", "[copyable_function]") {
	free_throwing ref1{func1};
	REQUIRE(ref1() == 0);
	const_free_throwing cref1{func1};
	REQUIRE(cref1() == 0);
	free_noexcept ref2{func2};
	REQUIRE(ref2() == 1);
	const_free_noexcept cref2{func2};
	REQUIRE(cref2() == 1);
}

TEST_CASE("copyable_function free copyable_function ptr", "[copyable_function]") {
	free_throwing ref1{&func1};
	REQUIRE(ref1() == 0);
	const_free_throwing cref1{&func1};
	REQUIRE(cref1() == 0);
	free_noexcept ref2{&func2};
	REQUIRE(ref2() == 1);
	const_free_noexcept cref2{&func2};
	REQUIRE(cref2() == 1);
}

TEST_CASE("copyable_function member copyable_function ptr", "[copyable_function]") { //TODO: more substantial unit test
	struct X {
		int val;

		int func(int) { return val; }
	} x{10};

	p2548::copyable_function<int(X *, int)> f{&X::func};

	REQUIRE(f(&x, 1) == x.val);

	p2548::copyable_function<int(X*)> m{&X::val};
	REQUIRE(m(&x) == x.val);
}

TEST_CASE("copyable_function functor", "[copyable_function]")  {
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

namespace {
	class small_func {
		int val;
	public:
		small_func(int val) noexcept : val{val} {}
		auto operator()() const -> int { return val; }
	};
	static_assert(sizeof(small_func) <= sizeof(const_free_throwing));

	class big_func {
		int val;
		int buffer[10];
	public:
		big_func(int val) noexcept : val{val} {}
		auto operator()() const -> int { return val; }
	};
	static_assert(sizeof(big_func) > sizeof(const_free_throwing));
}

TEST_CASE("copyable_function copying", "[copyable_function]") {
	//EMPTY
	const_free_throwing mf0;
	REQUIRE(!mf0);
	const_free_throwing f0{mf0};
	REQUIRE(!f0);
	REQUIRE(!mf0);

	//FREE FUNC
	const_free_throwing mf1{func1};
	REQUIRE(mf1);
	const_free_throwing f1{mf1};
	REQUIRE(f1);
	REQUIRE(mf1);

	//FREE FUNC PTR
	const_free_throwing mf2{&func1};
	REQUIRE(mf2);
	const_free_throwing f2{mf2};
	REQUIRE(f2);
	REQUIRE(mf2);

	//SOO
	const_free_throwing mf3{std::in_place_type<small_func>, 123};
	REQUIRE(mf3);
	const_free_throwing f3{mf3};
	REQUIRE(f3);
	REQUIRE(mf3);

	//noSOO
	const_free_throwing mf4{std::in_place_type<big_func>, 123};
	REQUIRE(mf4);
	const_free_throwing f4{mf4};
	REQUIRE(f4);
	REQUIRE(mf4);

	//TODO: copy-assignment
}

TEST_CASE("copyable_function moved-from state", "[copyable_function]") {
	//EMPTY
	const_free_throwing mf0;
	REQUIRE(!mf0);
	const_free_throwing f0{std::move(mf0)};
	REQUIRE(!f0);
	REQUIRE(!mf0);

	//FREE FUNC
	const_free_throwing mf1{func1};
	REQUIRE(mf1);
	const_free_throwing f1{std::move(mf1)};
	REQUIRE(f1);
	REQUIRE(mf1);

	//FREE FUNC PTR
	const_free_throwing mf2{&func1};
	REQUIRE(mf2);
	const_free_throwing f2{std::move(mf2)};
	REQUIRE(f2);
	REQUIRE(mf2);

	//SOO
	const_free_throwing mf3{std::in_place_type<small_func>, 123};
	REQUIRE(mf3);
	const_free_throwing f3{std::move(mf3)};
	REQUIRE(f3);
	REQUIRE(mf3);

	//noSOO
	const_free_throwing mf4{std::in_place_type<big_func>, 123};
	REQUIRE(mf4);
	const_free_throwing f4{std::move(mf4)};
	REQUIRE(f4);
	REQUIRE(!mf4); //move of nonSOO => invalidates original copyable_function
}

namespace {
	int func3() { return 2; }
}

TEST_CASE("copyable_function swapping", "[copyable_function]") {
	//EMPTY swap EMPTY
	const_free_throwing f0, f1;
	swap(f0, f1);
	REQUIRE(!f0);
	REQUIRE(!f1);

	//EMPTY swap FREE FUNC
	const_free_throwing f2, f3{func1};
	swap(f2, f3);
	REQUIRE(!f3);
	REQUIRE(f2);
	REQUIRE(f2() == 0);

	//EMPTY swap SOO
	const_free_throwing f4, f5{std::in_place_type<small_func>, 1234};
	swap(f4, f5);
	REQUIRE(!f5);
	REQUIRE(f4);
	REQUIRE(f4() == 1234);

	//EMPTY swap noSOO
	const_free_throwing f6, f7{std::in_place_type<big_func>, 56789};
	swap(f6, f7);
	REQUIRE(!f7);
	REQUIRE(f6);
	REQUIRE(f6() == 56789);

	//FREE FUNC swap FREE FUNC
	const_free_throwing f8{func1}, f9{func3};
	swap(f8, f9);
	REQUIRE(f8);
	REQUIRE(f8() == 2);
	REQUIRE(f9);
	REQUIRE(f9() == 0);

	//FREE FUNC swap SOO
	const_free_throwing f10{func1}, f11{std::in_place_type<small_func>, 10};
	swap(f10, f11);
	REQUIRE(f10);
	REQUIRE(f10() == 10);
	REQUIRE(f11);
	REQUIRE(f11() == 0);

	//FREE FUNC swap noSOO
	const_free_throwing f12{func1}, f13{std::in_place_type<big_func>, 10};
	swap(f12, f13);
	REQUIRE(f12);
	REQUIRE(f12() == 10);
	REQUIRE(f13);
	REQUIRE(f13() == 0);

	//noSOO swap noSOO
	const_free_throwing f14{std::in_place_type<big_func>, 1}, f15{std::in_place_type<big_func>, 2};
	swap(f14, f15);
	REQUIRE(f14);
	REQUIRE(f14() == 2);
	REQUIRE(f15);
	REQUIRE(f15() == 1);

	//noSOO swap SOO
	const_free_throwing f16{std::in_place_type<big_func>, 10}, f17{std::in_place_type<small_func>, 99};
	swap(f16, f17);
	REQUIRE(f16);
	REQUIRE(f16() == 99);
	REQUIRE(f17);
	REQUIRE(f17() == 10);

	//SSO swap SSO
	const_free_throwing f18{std::in_place_type<small_func>, 17}, f19{std::in_place_type<small_func>, 50};
	swap(f18, f19);
	REQUIRE(f18);
	REQUIRE(f18() == 50);
	REQUIRE(f19);
	REQUIRE(f19() == 17);
}

namespace {
	struct functor0 { void operator()(int) {} };
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor0>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor0>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor0>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor0>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor0>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor0>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor0>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor0>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor0>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor0>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor0>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor0>);
}
namespace {
	struct functor1 { void operator()(int) & {} };
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor1>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor1>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor1>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor1>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor1>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor1>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor1>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor1>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor1>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor1>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor1>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor1>);
}
namespace {
	struct functor2 { void operator()(int) && {} };
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor2>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor2>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor2>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor2>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor2>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor2>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor2>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor2>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor2>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor2>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor2>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor2>);
}
namespace {
	struct functor3 { void operator()(int) const {} };
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor3>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor3>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor3>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor3>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor3>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor3>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor3>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor3>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor3>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor3>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor3>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor3>);
}
namespace {
	struct functor4 {
		void operator()(int) const & {}
		void operator()(int) const && =delete;
	};
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor4>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor4>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor4>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor4>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor4>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor4>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor4>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor4>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor4>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor4>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor4>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor4>);
}
namespace {
	struct functor5 { void operator()(int) const && {} };
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor5>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor5>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor5>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor5>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor5>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor5>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor5>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor5>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor5>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor5>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor5>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor5>);
}
namespace {
	struct functor6 { void operator()(int) noexcept {} };
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor6>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor6>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor6>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor6>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor6>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor6>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor6>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor6>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor6>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor6>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor6>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor6>);
}
namespace {
	struct functor7 { void operator()(int) & noexcept {} };
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor7>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor7>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor7>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor7>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor7>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor7>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor7>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor7>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor7>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor7>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor7>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor7>);
}
namespace {
	struct functor8 { void operator()(int) && noexcept {} };
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor8>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor8>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor8>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor8>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor8>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor8>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor8>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor8>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor8>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor8>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor8>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor8>);
}
namespace {
	struct functor9 { void operator()(int) const noexcept {} };
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor9>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor9>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor9>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor9>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor9>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor9>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor9>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor9>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor9>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor9>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor9>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor9>);
}
namespace {
	struct functor10 {
		void operator()(int) const & noexcept {}
		void operator()(int) const && noexcept =delete;
	};
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor10>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor10>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor10>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor10>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor10>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor10>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor10>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor10>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor10>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor10>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor10>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor10>);
}
namespace {
	struct functor11 { void operator()(int) const && noexcept {} };
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)                  >, functor11>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const            >, functor11>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &          >, functor11>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &          >, functor11>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       &&         >, functor11>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const &&         >, functor11>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)          noexcept>, functor11>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const    noexcept>, functor11>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int)       &  noexcept>, functor11>);
	static_assert(!std::is_constructible_v<p2548::copyable_function<void(int) const &  noexcept>, functor11>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int)       && noexcept>, functor11>);
	static_assert( std::is_constructible_v<p2548::copyable_function<void(int) const && noexcept>, functor11>);
}
