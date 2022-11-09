
//          Copyright Michael Florian Hava.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <utility>
#include <type_traits>

namespace p2548 {
	static_assert(sizeof(void *) == sizeof(void(*)()));
	static_assert(sizeof(void *) == sizeof(void(*)() noexcept));

	namespace internal_function_ref {
		template<typename T>
		using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>; //TODO: [C++20] replace with std::remove_cvref_t

		template<typename>
		struct traits;

		template<typename Result, typename... Args>
		struct traits<Result(Args...)> final {
			using const_ = std::false_type;
			using noexcept_ = std::false_type;
			using dispatch_type = Result(*)(void *, Args...);

			template<typename T>
			static
			auto functor(void * ctx, Args... args) -> Result { return std::invoke_r<Result>(*reinterpret_cast<T *>(ctx), std::forward<Args>(args)...); }

			template<typename... T>
			static
			constexpr
			bool is_invocable_using{std::is_invocable_r_v<Result, T..., Args...>};
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const> final {
			using const_ = std::true_type;
			using noexcept_ = std::false_type;
			using dispatch_type = Result(*)(void *, Args...);

			template<typename T>
			static
			auto functor(void * ctx, Args... args) -> Result { return std::invoke_r<Result>(*reinterpret_cast<const T *>(ctx), std::forward<Args>(args)...); }

			template<typename... T>
			static
			constexpr
			bool is_invocable_using{std::is_invocable_r_v<Result, T..., Args...>};
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) noexcept> final {
			using const_ = std::false_type;
			using noexcept_ = std::true_type;
			using dispatch_type = Result(*)(void *, Args...) noexcept;

			template<typename T>
			static
			auto functor(void * ctx, Args... args) noexcept -> Result { return std::invoke_r<Result>(*reinterpret_cast<T *>(ctx), std::forward<Args>(args)...); }

			template<typename... T>
			static
			constexpr
			bool is_invocable_using{std::is_nothrow_invocable_r_v<Result, T..., Args...>};
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const noexcept> final {
			using const_ = std::true_type;
			using noexcept_ = std::true_type;
			using dispatch_type = Result(*)(void *, Args...) noexcept;

			template<typename T>
			static
			auto functor(void * ctx, Args... args) noexcept -> Result { return std::invoke_r<Result>(*reinterpret_cast<const T *>(ctx), std::forward<Args>(args)...); }

			template<typename... T>
			static
			constexpr
			bool is_invocable_using{std::is_nothrow_invocable_r_v<Result, T..., Args...>};
		};


		template<typename Impl, typename Signature>
		struct function_call;

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...)> {
			auto operator()(Args... args) const -> Result { //TODO: [C++23] use deducing this instead of CRTP
				auto & self{*static_cast<const Impl *>(this)};
				return self.dispatch(self.ptr, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) const> {
			auto operator()(Args... args) const -> Result { //TODO: [C++23] use deducing this instead of CRTP
				auto & self{*static_cast<const Impl *>(this)};
				return self.dispatch(self.ptr, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) noexcept> {
			auto operator()(Args... args) const noexcept -> Result { //TODO: [C++23] use deducing this instead of CRTP
				auto & self{*static_cast<const Impl *>(this)};
				return self.dispatch(self.ptr, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) const noexcept> {
			auto operator()(Args... args) const noexcept -> Result { //TODO: [C++23] use deducing this instead of CRTP
				auto & self{*static_cast<const Impl *>(this)};
				return self.dispatch(self.ptr, std::forward<Args>(args)...);
			}
		};
	}

	//! @brief non-owning reference to a function (either a plain function or a functor)
	//! @tparam Signature function signature of the referenced functor (including potential const and noexcept qualifiers)
	template<typename... Signature>
	struct function_ref;

	template<typename Signature>
	class function_ref<Signature> final : internal_function_ref::function_call<function_ref<Signature>, Signature> {
		using traits = internal_function_ref::traits<Signature>;
		friend internal_function_ref::function_call<function_ref, Signature>;

		static
		constexpr
		bool noexcept_{typename traits::noexcept_{}}; //TODO: remove

		template<typename T>
		using const_ = std::conditional_t<typename traits::const_{}, const T, T>;

		void * ptr;
		typename traits::dispatch_type dispatch;

		template<typename... T>
		static
		constexpr
		bool is_invocable_using{traits::template is_invocable_using<T...>};
	public:
		template<typename F>
		requires(std::is_function_v<F> && is_invocable_using<F>)
		function_ref(F * func) noexcept {
			//TODO: [C++??] precondition(func);
			ptr = reinterpret_cast<void *>(func);
			dispatch = traits::template functor<F>;
		}

		template<typename F, typename T = std::remove_reference_t<F>>
		requires(!std::is_same_v<function_ref, internal_function_ref::remove_cvref_t<F>> && !std::is_member_pointer_v<T> && is_invocable_using<const_<T> &>)
		function_ref(F && func) noexcept {
			ptr = reinterpret_cast<void *>(std::addressof(func));
			dispatch = traits::template functor<T>;
		}

		constexpr
		function_ref(const function_ref &) noexcept =default;
		constexpr
		auto operator=(const function_ref &) noexcept -> function_ref & =default;

		template<typename T>
		requires(!std::is_same_v<function_ref, T> && !std::is_pointer_v<T>)
		auto operator=(T) -> function_ref & =delete;

		using internal_function_ref::function_call<function_ref, Signature>::operator();
	};

	template<typename F>
	function_ref(F *) -> function_ref<F>;

	namespace internal_function_ref {
		template<typename Signature>
		struct deduce_signature;

		template<typename Result, typename Class>
		struct deduce_signature<Result Class::*> final { using type = Result(); };

		template<typename Result, typename Class, typename... Args>
		struct deduce_signature<Result(Class::*)(Args...)> final { using type = Result(Args...); };

		template<typename Result, typename Class, typename... Args>
		struct deduce_signature<Result(Class::*)(Args...) const> final { using type = Result(Args...); };

		template<typename Result, typename Class, typename... Args>
		struct deduce_signature<Result(Class::*)(Args...) noexcept> final { using type = Result(Args...) noexcept; };

		template<typename Result, typename Class, typename... Args>
		struct deduce_signature<Result(Class::*)(Args...) const noexcept> final { using type = Result(Args...) noexcept; };

		template<typename Result, typename Class, typename... Args>
		struct deduce_signature<Result(Class::*)(Args...) &> final { using type = Result(Args...); };

		template<typename Result, typename Class, typename... Args>
		struct deduce_signature<Result(Class::*)(Args...) const &> final { using type = Result(Args...); };

		template<typename Result, typename Class, typename... Args>
		struct deduce_signature<Result(Class::*)(Args...) & noexcept> final { using type = Result(Args...) noexcept; };

		template<typename Result, typename Class, typename... Args>
		struct deduce_signature<Result(Class::*)(Args...) const & noexcept> final { using type = Result(Args...) noexcept; };

		template<typename Result, typename Class, typename... Args>
		struct deduce_signature<Result(*)(Class, Args...)> final { using type = Result(Args...); };

		template<typename Result, typename Class, typename... Args>
		struct deduce_signature<Result(*)(Class, Args...) noexcept> final { using type = Result(Args...) noexcept; };
	}

	//TODO: static_assert(sizeof(function_ref<T>) == 2 * sizeof(void *));
}
