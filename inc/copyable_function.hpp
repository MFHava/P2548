
//          Copyright Michael Florian Hava.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <utility>
#include <type_traits>

namespace p2548 {
	namespace internal {
		enum class refness { none, lvalue, rvalue, };

		using none_type = std::integral_constant<refness, refness::none>;
		using lvalue_type = std::integral_constant<refness, refness::lvalue>;
		using rvalue_type = std::integral_constant<refness, refness::rvalue>;


		template<typename T, refness Ref>
		struct add_refness;

		template<typename T>
		struct add_refness<T, refness::none> final { using type = T; };

		template<typename T>
		struct add_refness<T, refness::lvalue> final { using type = T &; };

		template<typename T>
		struct add_refness<T, refness::rvalue> final { using type = T &&; };

		template<typename T, refness Ref>
		using add_refness_t = typename add_refness<T, Ref>::type;


		template<typename>
		struct traits;

		template<typename Result, typename... Args>
		struct traits<Result(Args...)> final {
			using function = Result(Args...);
			using const_ = std::false_type;
			using noexcept_ = std::false_type;
			using ref = none_type;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const> final {
			using function = Result(Args...);
			using const_ = std::true_type;
			using noexcept_ = std::false_type;
			using ref = none_type;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) noexcept> final {
			using function = Result(Args...);
			using const_ = std::false_type;
			using noexcept_ = std::true_type;
			using ref = none_type;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const noexcept> final {
			using function = Result(Args...);
			using const_ = std::true_type;
			using noexcept_ = std::true_type;
			using ref = none_type;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) &> final {
			using function = Result(Args...);
			using const_ = std::false_type;
			using noexcept_ = std::false_type;
			using ref = lvalue_type;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const &> final {
			using function = Result(Args...);
			using const_ = std::true_type;
			using noexcept_ = std::false_type;
			using ref = lvalue_type;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) & noexcept> final {
			using function = Result(Args...);
			using const_ = std::false_type;
			using noexcept_ = std::true_type;
			using ref = lvalue_type;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const & noexcept> final {
			using function = Result(Args...);
			using const_ = std::true_type;
			using noexcept_ = std::true_type;
			using ref = lvalue_type;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) &&> final {
			using function = Result(Args...);
			using const_ = std::false_type;
			using noexcept_ = std::false_type;
			using ref = rvalue_type;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const &&> final {
			using function = Result(Args...);
			using const_ = std::true_type;
			using noexcept_ = std::false_type;
			using ref = rvalue_type;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) && noexcept> final {
			using function = Result(Args...);
			using const_ = std::false_type;
			using noexcept_ = std::true_type;
			using ref = rvalue_type;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const && noexcept> final {
			using function = Result(Args...);
			using const_ = std::true_type;
			using noexcept_ = std::true_type;
			using ref = rvalue_type;
		};


		template<typename>
		struct is_in_place_type_t_specialization : std::false_type {};

		template<typename T>
		struct is_in_place_type_t_specialization<std::in_place_type_t<T>> : std::true_type {};

		template<typename T>
		inline
		constexpr
		bool is_in_place_type_t_specialization_v{is_in_place_type_t_specialization<T>::value};


		template<typename R, typename F, typename... Args>
		requires std::is_invocable_r_v<R, F, Args...>
		constexpr
		auto invoke_r(F && f, Args &&... args) noexcept(std::is_nothrow_invocable_r_v<R, F, Args...>) { //TODO: [C++23] replace with std::invoke_r
			if constexpr(std::is_void_v<R>) std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
			else return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
		}
	}

	//! @brief copyable function wrapper
	//! @tparam Signature function signature of the contained functor (including potential const-, ref- and noexcept-qualifiers)
	template<typename Signature, typename = typename internal::traits<Signature>::function>
	class copyable_function;

	namespace internal {
		template<typename...>
		struct is_function_specialization : std::false_type {};

		template<typename... Ts>
		struct is_function_specialization<copyable_function<Ts...>> : std::true_type {};

		template<typename... Ts>
		inline
		constexpr
		bool is_function_specialization_v{is_function_specialization<Ts...>::value};
	}

	template<typename Signature, typename Result, typename... Args>
	class copyable_function<Signature, Result(Args...)> final {
		using traits = internal::traits<Signature>;

		using ref_ = typename traits::ref;

		static
		constexpr
		bool const_{typename traits::const_{}},
		     noexcept_{typename traits::noexcept_{}},
		     none{ref_{} == internal::refness::none},
		     lvalue{ref_{} == internal::refness::lvalue},
		     rvalue{ref_{} == internal::refness::rvalue};

		template<typename T>
		using add_const = std::conditional_t<const_, const T, T>;

		template<typename T>
		using add_quals = internal::add_refness_t<add_const<T>, ref_::value>;

		template<typename T>
		using add_inv_quals = internal::add_refness_t<add_const<T>, ref_::value == internal::refness::none ? internal::refness::lvalue : ref_::value>;

		union storage_t {
			void * ptr;
			char sbo[sizeof(void * ) * 3];
		};
		const struct vtable final {
			auto (*manage)(storage_t *, storage_t *, bool) noexcept -> bool; //TODO: add copying? OR introduce new copy-operation...
			auto (*dispatch)(add_const<storage_t> *, Args...) noexcept(noexcept_) -> Result;

			//TODO: copy based on manage?
			void dtor(storage_t * self) const noexcept { manage(self, nullptr, false); }
			auto move(storage_t * from, storage_t * to) const noexcept -> bool { return manage(from, to, false); } //returns true => heap-allocated object was moved; from is now empty => storage & vptr must be reset...
			void destructive_move(storage_t * from, storage_t * to) const noexcept { manage(from, to, true); }
		} * vptr;
		storage_t storage;

		template<typename T, bool SBO>
		static
		auto invoke(add_const<storage_t> * ctx, Args... args) noexcept(noexcept_) -> Result { return internal::invoke_r<Result>(static_cast<add_inv_quals<T>>(*reinterpret_cast<add_const<T> *>(SBO ? ctx->sbo : ctx->ptr)), std::forward<Args>(args)...); }

		template<typename T, typename... A>
		void init(A &&... args) {
			static_assert(std::is_nothrow_destructible_v<T>);
			//TODO: static_assert(std::is_copy_constructible_v<T>);
			//TODO: static_assert(std::is_copy_assignable_v<T>);

			if constexpr(constexpr auto sbo{sizeof(T) <= sizeof(storage_t::sbo) && std::is_nothrow_move_constructible_v<T>}; sbo) { //TODO: must be nothrow_copy_constructible unless SBO will be extremely problematic
				static constexpr vtable vtable{
					+[](storage_t * from, storage_t * to, bool destructive) noexcept {
						if(to) new(to->sbo) T{std::move(*reinterpret_cast<T *>(from->sbo))};
						if(!to || destructive) reinterpret_cast<T *>(from->sbo)->~T();
						return false;
					},
					&invoke<T, sbo>
				};
				vptr = &vtable;
				new(storage.sbo) T{std::forward<A>(args)...};
			} else {
				static constexpr vtable vtable{
					+[](storage_t * from, storage_t * to, bool) noexcept {
						if(to) to->ptr = std::exchange(from->ptr, nullptr);
						else delete reinterpret_cast<T *>(from->ptr);
						return true;
					},
					&invoke<T, sbo>
				};
				vptr = &vtable;
				storage.ptr = new T{std::forward<A>(args)...};
			}
		}

		void init_empty() noexcept {
			static constexpr vtable vtable{
				+[](storage_t *, storage_t * to, bool) noexcept {
					if(to) to->ptr = nullptr;
					return false;
				},
				nullptr
			};
			vptr = &vtable;
		}

		template<typename VT>
		static
		constexpr
		bool is_callable_from{noexcept_ ? std::is_nothrow_invocable_r_v<Result, add_quals<VT>, Args...> && std::is_nothrow_invocable_r_v<Result, add_inv_quals<VT>, Args...>
		                                : std::is_invocable_r_v<Result, add_quals<VT>, Args...> && std::is_invocable_r_v<Result, add_inv_quals<VT>, Args...>};
	public:
		copyable_function() noexcept { init_empty(); }
		copyable_function(std::nullptr_t) noexcept : copyable_function{} {}

		template<typename F>
		requires(!std::is_same_v<copyable_function, std::remove_cvref_t<F>> && !internal::is_in_place_type_t_specialization_v<std::remove_cvref_t<F>> && is_callable_from<std::decay_t<F>>)
		copyable_function(F && func) {
			using VT = std::decay_t<F>;
			static_assert(std::is_constructible_v<VT, F>);
			if constexpr(std::is_function_v<std::remove_pointer_t<F>> || std::is_member_pointer_v<F> || internal::is_function_specialization_v<std::remove_cvref_t<F>>)
				if(!func) {
					init_empty();
					return;
				}
			init<VT>(std::forward<F>(func));
		}

		template<typename T, typename... A>
		requires(std::is_constructible_v<std::decay_t<T>, A &&...> && is_callable_from<std::decay_t<T>>)
		explicit
		copyable_function(std::in_place_type_t<T>, A &&... args) {
			using VT = std::decay_t<T>;
			static_assert(std::is_same_v<VT, std::decay_t<T>>);
			init<VT>(std::forward<A>(args)...);
		}

		//TODO: copy-ctor

		copyable_function(copyable_function && other) noexcept : vptr{other.vptr} { if(other.vptr->move(&other.storage, &storage)) other.init_empty(); }

		//TODO: copy-assign

		auto operator=(copyable_function && other) noexcept -> copyable_function & {
			if(this != &other) {
				vptr->dtor(&storage);
				vptr = other.vptr;
				if(other.vptr->move(&other.storage, &storage)) other.init_empty();
			}
			return *this;
		}
		auto operator=(std::nullptr_t) noexcept -> copyable_function & {
			if(*this) copyable_function{}.swap(*this);
			return *this;
		}

		template<typename F>
		auto operator=(F && func) -> copyable_function & {
			copyable_function{std::forward<F>(func)}.swap(*this);
			return *this;
		}

		~copyable_function() noexcept { vptr->dtor(&storage); }

		explicit
		operator bool() const noexcept { return vptr->dispatch; }

		auto operator()(Args... args) noexcept(noexcept_) -> Result requires(!const_ && none) { return vptr->dispatch(&storage, std::forward<Args>(args)...); }
		auto operator()(Args... args) const noexcept(noexcept_) -> Result requires(const_ && none) { return vptr->dispatch(&storage, std::forward<Args>(args)...); }
		auto operator()(Args... args) & noexcept(noexcept_) -> Result requires(!const_ && lvalue) { return vptr->dispatch(&storage, std::forward<Args>(args)...); }
		auto operator()(Args... args) const & noexcept(noexcept_) -> Result requires(const_ && lvalue) { return vptr->dispatch(&storage, std::forward<Args>(args)...); }
		auto operator()(Args... args) && noexcept(noexcept_) -> Result requires(!const_ && rvalue) { return vptr->dispatch(&storage, std::forward<Args>(args)...); }
		auto operator()(Args... args) const && noexcept(noexcept_) -> Result requires(const_ && rvalue) { return vptr->dispatch(&storage, std::forward<Args>(args)...); }

		void swap(copyable_function & other) noexcept {
			if(this == &other) return;
			storage_t tmp;
			vptr->destructive_move(&storage, &tmp);
			other.vptr->destructive_move(&other.storage, &storage);
			vptr->destructive_move(&tmp, &other.storage);
			std::swap(vptr, other.vptr);
		}
		friend
		void swap(copyable_function & lhs, copyable_function & rhs) noexcept { lhs.swap(rhs); }

		friend
		auto operator==(const copyable_function & self, std::nullptr_t) noexcept -> bool { return !self; }
	};
}
