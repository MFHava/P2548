
//          Copyright Michael Florian Hava.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <utility>
#include <type_traits>

namespace p2548 {
	//! @brief copyable function wrapper
	//! @tparam Signature function signature of the contained functor (including potential const-, ref- and noexcept-qualifiers)
	template<typename... Signature>
	class copyable_function;

	namespace internal {
		template<typename R, typename F, typename... Args>
		requires std::is_invocable_r_v<R, F, Args...>
		constexpr
		auto invoke_r(F && f, Args &&... args) noexcept(std::is_nothrow_invocable_r_v<R, F, Args...>) { //TODO: [C++23] replace with std::invoke_r
			if constexpr(std::is_void_v<R>) std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
			else return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
		}


		union storage_t {
			void * ptr;
			char sbo[sizeof(void * ) * 3];
		};


		template<typename Impl, typename>
		struct traits;

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...)> {
			using result_type = Result;
			using dispatch_type = Result(*)(storage_t *, Args...);

			auto operator()(Args... args) -> Result {
				auto & self{*static_cast<Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(storage_t * ctx, Args... args) -> Result { return invoke_r<Result, T &>(*reinterpret_cast<T *>(SBO ? ctx->sbo : ctx->ptr), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_invocable_r_v<Result, VT, Args...> && std::is_invocable_r_v<Result, VT &, Args...>};
		};

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...) const> {
			using result_type = Result;
			using dispatch_type = Result(*)(const storage_t *, Args...);

			auto operator()(Args... args) const -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(const storage_t * ctx, Args... args) -> Result { return invoke_r<Result, const T &>(*reinterpret_cast<const T *>(SBO ? ctx->sbo : ctx->ptr), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_invocable_r_v<Result, const VT, Args...> && std::is_invocable_r_v<Result, const VT &, Args...>};
		};

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...) noexcept> {
			using result_type = Result;
			using dispatch_type = Result(*)(storage_t *, Args...) noexcept;

			auto operator()(Args... args) noexcept -> Result {
				auto & self{*static_cast<Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(storage_t * ctx, Args... args) noexcept -> Result { return invoke_r<Result, T &>(*reinterpret_cast<T *>(SBO ? ctx->sbo : ctx->ptr), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_nothrow_invocable_r_v<Result, VT, Args...> && std::is_nothrow_invocable_r_v<Result, VT &, Args...>};
		};

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...) const noexcept> {
			using result_type = Result;
			using dispatch_type = Result(*)(const storage_t *, Args...) noexcept;

			auto operator()(Args... args) const noexcept -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(const storage_t * ctx, Args... args) noexcept -> Result { return invoke_r<Result, const T &>(*reinterpret_cast<const T *>(SBO ? ctx->sbo : ctx->ptr), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_nothrow_invocable_r_v<Result, const VT, Args...> && std::is_nothrow_invocable_r_v<Result, const VT &, Args...>};
		};

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...) &> {
			using result_type = Result;
			using dispatch_type = Result(*)(storage_t *, Args...);

			auto operator()(Args... args) & -> Result {
				auto & self{*static_cast<Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(storage_t * ctx, Args... args) -> Result { return invoke_r<Result, T &>(*reinterpret_cast<T *>(SBO ? ctx->sbo : ctx->ptr), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_invocable_r_v<Result, VT &, Args...> && std::is_invocable_r_v<Result, VT &, Args...>};
		};

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...) const &> {
			using result_type = Result;
			using dispatch_type = Result(*)(const storage_t *, Args...);

			auto operator()(Args... args) const & -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(const storage_t * ctx, Args... args) -> Result { return invoke_r<Result, const T &>(*reinterpret_cast<const T *>(SBO ? ctx->sbo : ctx->ptr), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_invocable_r_v<Result, const VT &, Args...> && std::is_invocable_r_v<Result, const VT &, Args...>};
		};

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...) & noexcept> {
			using result_type = Result;
			using dispatch_type = Result(*)(storage_t *, Args...) noexcept;

			auto operator()(Args... args) & noexcept -> Result {
				auto & self{*static_cast<Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(storage_t * ctx, Args... args) noexcept -> Result { return invoke_r<Result, T &>(*reinterpret_cast<T *>(SBO ? ctx->sbo : ctx->ptr), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_nothrow_invocable_r_v<Result, VT &, Args...> && std::is_nothrow_invocable_r_v<Result, VT &, Args...>};
		};

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...) const & noexcept> {
			using result_type = Result;
			using dispatch_type = Result(*)(const storage_t *, Args...) noexcept;

			auto operator()(Args... args) const & noexcept -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(const storage_t * ctx, Args... args) noexcept -> Result { return invoke_r<Result, const T &>(*reinterpret_cast<const T *>(SBO ? ctx->sbo : ctx->ptr), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_nothrow_invocable_r_v<Result, const VT &, Args...> && std::is_nothrow_invocable_r_v<Result, const VT &, Args...>};
		};

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...) &&> {
			using result_type = Result;
			using dispatch_type = Result(*)(storage_t *, Args...);

			auto operator()(Args... args) && -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(storage_t * ctx, Args... args) -> Result { return invoke_r<Result, T &&>(std::move(*reinterpret_cast<T *>(SBO ? ctx->sbo : ctx->ptr)), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_invocable_r_v<Result, VT &&, Args...> && std::is_invocable_r_v<Result, VT &&, Args...>};
		};

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...) const &&> {
			using result_type = Result;
			using dispatch_type = Result(*)(const storage_t *, Args...);

			auto operator()(Args... args) const && -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(const storage_t * ctx, Args... args) -> Result { return invoke_r<Result, const T &&>(std::move(*reinterpret_cast<const T *>(SBO ? ctx->sbo : ctx->ptr)), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_invocable_r_v<Result, const VT &&, Args...> && std::is_invocable_r_v<Result, const VT &&, Args...>};
		};

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...) && noexcept> {
			using result_type = Result;
			using dispatch_type = Result(*)(storage_t *, Args...) noexcept;

			auto operator()(Args... args) && noexcept -> Result {
				auto & self{*static_cast<Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(storage_t * ctx, Args... args) noexcept -> Result { return invoke_r<Result, T &&>(std::move(*reinterpret_cast<T *>(SBO ? ctx->sbo : ctx->ptr)), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_nothrow_invocable_r_v<Result, VT &&, Args...> && std::is_nothrow_invocable_r_v<Result, VT &&, Args...>};
		};

		template<typename Impl, typename Result, typename... Args>
		struct traits<Impl, Result(Args...) const && noexcept> {
			using result_type = Result;
			using dispatch_type = Result(*)(const storage_t *, Args...) noexcept;

			auto operator()(Args... args) const && noexcept -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}

			template<typename T, bool SBO>
			static
			auto invoke(const storage_t * ctx, Args... args) noexcept -> Result { return invoke_r<Result, const T &&>(std::move(*reinterpret_cast<const T *>(SBO ? ctx->sbo : ctx->ptr)), std::forward<Args>(args)...); }

			template<typename VT>
			static
			constexpr
			bool is_callable_from{std::is_nothrow_invocable_r_v<Result, const VT &&, Args...> && std::is_nothrow_invocable_r_v<Result, const VT &&, Args...>};
		};


		template<typename...>
		struct is_function_specialization : std::false_type {};

		template<typename... Ts>
		struct is_function_specialization<copyable_function<Ts...>> : std::true_type {};

		template<typename... Ts>
		inline
		constexpr
		bool is_function_specialization_v{is_function_specialization<Ts...>::value};


		template<typename>
		struct is_in_place_type_t_specialization : std::false_type {};

		template<typename T>
		struct is_in_place_type_t_specialization<std::in_place_type_t<T>> : std::true_type {};

		template<typename T>
		inline
		constexpr
		bool is_in_place_type_t_specialization_v{is_in_place_type_t_specialization<T>::value};
	}

	template<typename Signature>
	class copyable_function<Signature> : internal::traits<copyable_function<Signature>, Signature> {
		using traits = internal::traits<copyable_function, Signature>;
		friend traits;

		template<typename VT>
		static
		constexpr
		bool is_callable_from{traits::template is_callable_from<VT>};

		enum class mode { dtor, move, dmove, copy, };

		const struct vtable final {
			auto (*manage)(internal::storage_t *, internal::storage_t *, mode) -> bool;
			typename traits::dispatch_type dispatch;

			void dtor(internal::storage_t * self) const noexcept { manage(self, nullptr, mode::dtor); }
			auto move(internal::storage_t * from, internal::storage_t * to) const noexcept -> bool { return manage(from, to, mode::move); } //returns true => heap-allocated object was moved; from is now empty => storage & vptr must be reset...
			void destructive_move(internal::storage_t * from, internal::storage_t * to) const noexcept { manage(from, to, mode::dmove); }
			void copy(const internal::storage_t * from, internal::storage_t * to) const { manage(const_cast<internal::storage_t *>(from), to, mode::copy); }
		} * vptr;
		internal::storage_t storage;

		template<typename T, typename... A>
		void init(A &&... args) {
			static_assert(std::is_copy_constructible_v<T>);
			static_assert(std::is_nothrow_destructible_v<T>);

			if constexpr(constexpr auto sbo{sizeof(T) <= sizeof(internal::storage_t::sbo) && std::is_nothrow_move_constructible_v<T>}; sbo) {
				static constexpr vtable vtable{
					+[](internal::storage_t * from, internal::storage_t * to, mode m) {
						if(m == mode::copy) new(to->sbo) T{*reinterpret_cast<const T *>(from->sbo)};
						else { //noexcept:
							if(m == mode::move || m == mode::dmove) new(to->sbo) T{std::move(*reinterpret_cast<T *>(from->sbo))};
							if(m == mode::dtor || m == mode::dmove) reinterpret_cast<T *>(from->sbo)->~T();
						}
						return false;
					},
					&traits::template invoke<T, sbo>
				};
				vptr = &vtable;
				new(storage.sbo) T{std::forward<A>(args)...};
			} else {
				static constexpr vtable vtable{
					+[](internal::storage_t * from, internal::storage_t * to, mode m) {
						switch(m) {
							case mode::dtor: delete reinterpret_cast<T *>(from->ptr); break;
							case mode::move: //always "destructive" for heap allocated objects
							case mode::dmove:
								to->ptr = std::exchange(from->ptr, nullptr);
								break;
							case mode::copy: to->ptr = new T{*reinterpret_cast<const T *>(from->ptr)}; break;
						}
						return true;
					},
					&traits::template invoke<T, sbo>
				};
				vptr = &vtable;
				storage.ptr = new T{std::forward<A>(args)...};
			}
		}

		void init_empty() noexcept {
			static constexpr vtable vtable{
				+[](internal::storage_t *, internal::storage_t *, mode) { return false; },
				nullptr
			};
			vptr = &vtable;
		}
	public:
		using typename traits::result_type;

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

		template<typename T, typename U, typename... A>
		requires(std::is_constructible_v<std::decay_t<T>, A &&...> && is_callable_from<std::decay_t<T>>)
		explicit
		copyable_function(std::in_place_type_t<T>, std::initializer_list<U> ilist, A &&... args) {
			using VT = std::decay_t<T>;
			static_assert(std::is_same_v<VT, std::decay_t<T>>);
			init<VT>(ilist, std::forward<A>(args)...);
		}

		copyable_function(const copyable_function & other) : vptr{other.vptr} { other.vptr->copy(&other.storage, &storage); }

		copyable_function(copyable_function && other) noexcept : vptr{other.vptr} { if(other.vptr->move(&other.storage, &storage)) other.init_empty(); }

		auto operator=(const copyable_function & other) -> copyable_function & { //TODO: investigate for exception-safe alternative without double-buffering (add copy-is-noexcept to vtable?)
			copyable_function{other}.swap(*this);
			return *this;
		}

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

		using traits::operator();

		void swap(copyable_function & other) noexcept {
			if(this == &other) return;
			internal::storage_t tmp;
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
