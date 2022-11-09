
//          Copyright Michael Florian Hava.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <utility>
#include <functional>
#include <type_traits>

namespace p2548 {
	//! @brief move-only function wrapper
	//! @tparam Signature function signature of the contained functor (including potential const-, ref- and noexcept-qualifiers)
	template<typename... Signature>
	class move_only_function;


	//! @brief copyable function wrapper
	//! @tparam Signature function signature of the contained functor (including potential const-, ref- and noexcept-qualifiers)
	template<typename... Signature>
	class copyable_function;


	namespace internal_function {
		union storage_t {
			void * ptr;
			char sbo[sizeof(void * ) * 3];
		};


		enum class mode { dtor, destructive_move, copy, copy_is_nothrow, };


		template<typename T>
		inline
		constexpr
		bool sbo{sizeof(T) <= sizeof(storage_t::sbo) && std::is_nothrow_move_constructible_v<T>};


		template<bool Copyable, typename T>
		auto owning_manage(storage_t * from, storage_t * to, mode m) -> bool {
			if constexpr(sbo<T>) {
				switch(m) {
					case mode::copy:
						if constexpr(Copyable) new(to->sbo) T{*reinterpret_cast<const T *>(from->sbo)};
						else std::unreachable();
						break;
					case mode::destructive_move:
						new(to->sbo) T{std::move(*reinterpret_cast<T *>(from->sbo))};
						[[fallthrough]];
					case mode::dtor:
						reinterpret_cast<T *>(from->sbo)->~T();
						break;
					case mode::copy_is_nothrow: /*nop*/ break;
				}
				return std::is_nothrow_copy_constructible_v<T>;
			} else {
				switch(m) {
					case mode::dtor:
						delete reinterpret_cast<T *>(from->ptr);
						break;
					case mode::destructive_move:
						to->ptr = std::exchange(from->ptr, nullptr);
						break;
					case mode::copy:
						if constexpr(Copyable) to->ptr = new T{*reinterpret_cast<const T *>(from->ptr)};
						else std::unreachable();
						break;
					case mode::copy_is_nothrow: /*nop*/ break;
				}
				return false;
			}
		}


		inline
		auto nop_manage(storage_t *, storage_t *, mode) { return true; }


		template<bool Copyable, typename T, typename... A>
		void construct(storage_t & storage, A &&... args) {
			if constexpr(Copyable) static_assert(std::is_copy_constructible_v<T>);
			//PRECONDITION: std::is_nothrow_destructible_v<T>

			if constexpr(sbo<T>) new(storage.sbo) T{std::forward<A>(args)...};
			else storage.ptr = new T{std::forward<A>(args)...};
		}


		template<typename Traits>
		struct vtable final {
			bool (*manage)(storage_t *, storage_t *, mode);
			typename Traits::dispatch_type dispatch;

			void dtor(storage_t * self) const noexcept { manage(self, nullptr, mode::dtor); }
			void destructive_move(storage_t * from, storage_t * to) const noexcept { manage(from, to, mode::destructive_move); }
			void copy(const storage_t * from, storage_t * to) const { manage(const_cast<storage_t *>(from), to, mode::copy); }
			auto noexcept_copyable() const noexcept -> bool { return manage(nullptr, nullptr, mode::copy_is_nothrow); }

			static
			void move_ctor(const vtable *& lhs_vptr, storage_t & lhs_storage, const vtable *& rhs_vptr, storage_t & rhs_storage) noexcept {
				lhs_vptr = rhs_vptr;
				rhs_vptr->destructive_move(&rhs_storage, &lhs_storage);
				rhs_vptr = vtable::init_empty();
			}

			static
			void move_assign(const vtable *& lhs_vptr, storage_t & lhs_storage, const vtable *& rhs_vptr, storage_t & rhs_storage) noexcept {
				if(&lhs_storage == &rhs_storage) return;

				lhs_vptr->dtor(&lhs_storage);
				lhs_vptr = rhs_vptr;
				rhs_vptr->destructive_move(&rhs_storage, &lhs_storage);
				rhs_vptr = vtable::init_empty();
			}

			static
			void swap(const vtable *& lhs_vptr, storage_t & lhs_storage, const vtable *& rhs_vptr, storage_t & rhs_storage) noexcept {
				if(&lhs_storage == &rhs_storage) return;

				storage_t tmp;
				lhs_vptr->destructive_move(&lhs_storage, &tmp);
				rhs_vptr->destructive_move(&rhs_storage, &lhs_storage);
				lhs_vptr->destructive_move(&tmp, &rhs_storage);
				std::swap(lhs_vptr, rhs_vptr);
			}

			template<bool Copyable, typename T, typename... A>
			static
			auto init_functor(storage_t & storage, A &&... args) -> const vtable * {
				construct<Copyable, T>(storage, std::forward<A>(args)...);
				static constexpr vtable vtable{&owning_manage<Copyable, T>, &Traits::template functor<T, sbo<T>>};
				return &vtable;
			}

			static
			auto init_empty() noexcept -> const vtable * {
				static constexpr vtable vtable{&nop_manage, nullptr};
				return &vtable;
			}
		};


		template<bool Const, bool Noexcept, bool Move, typename Result, typename... Args>
		class invoker {
			template<typename T>
			using const_ = std::conditional_t<Const, const T, T>;

			template<typename T>
			using move_ = std::conditional_t<Move, T &&, T &>;

			template<typename T>
			static
			auto move(T && t) noexcept -> decltype(auto) {
				if constexpr(Move) return std::move(t);
				else return (t);
			}

			template<typename T, bool SBO>
			static
			auto get(const_<storage_t> * ctx) noexcept -> move_<const_<T>> { return move(*reinterpret_cast<const_<T> *>(SBO ? ctx->sbo : ctx->ptr)); }
		public:
			using dispatch_type = std::conditional_t<Noexcept, Result(*)(const_<storage_t> *, Args...) noexcept, Result(*)(const_<storage_t> *, Args...)>;

			template<typename T, bool SBO>
			static
			auto functor(const_<storage_t> * ctx, Args... args) noexcept(Noexcept) -> Result { return std::invoke_r<Result>(get<T, SBO>(ctx), std::forward<Args>(args)...); }
		};


		template<typename Signature>
		struct invocable_using;

		template<typename Result, typename... Args>
		struct invocable_using<Result(Args...)> {
			template<typename... T>
			static
			constexpr
			bool is_invocable_using{std::is_invocable_r_v<Result, T..., Args...>};
		};

		template<typename Result, typename... Args>
		struct invocable_using<Result(Args...) noexcept> {
			template<typename... T>
			static
			constexpr
			bool is_invocable_using{std::is_nothrow_invocable_r_v<Result, T..., Args...>};
		};


		template<typename>
		struct traits;

		template<typename Result, typename... Args>
		struct traits<Result(Args...)> final : invocable_using<Result(Args...)>, invoker<false, false, false, Result, Args...> {
			template<typename VT>
			using quals = VT;

			template<typename VT>
			using inv_quals = VT &;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const> final : invocable_using<Result(Args...)>, invoker<true, false, false, Result, Args...> {
			template<typename VT>
			using quals = const VT;

			template<typename VT>
			using inv_quals = const VT &;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) noexcept> final : invocable_using<Result(Args...) noexcept>, invoker<false, true, false, Result, Args...> {
			template<typename VT>
			using quals = VT;

			template<typename VT>
			using inv_quals = VT &;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const noexcept> final : invocable_using<Result(Args...) noexcept>, invoker<true, true, false, Result, Args...> {
			template<typename VT>
			using quals = const VT;

			template<typename VT>
			using inv_quals = const VT &;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) &> final : invocable_using<Result(Args...)>, invoker<false, false, false, Result, Args...> {
			template<typename VT>
			using quals = VT &;

			template<typename VT>
			using inv_quals = quals<VT>;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const &> final : invocable_using<Result(Args...)>, invoker<true, false, false, Result, Args...> {
			template<typename VT>
			using quals = const VT &;

			template<typename VT>
			using inv_quals = quals<VT>;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) & noexcept> final : invocable_using<Result(Args...) noexcept>, invoker<false, true, false, Result, Args...> {
			template<typename VT>
			using quals = VT &;

			template<typename VT>
			using inv_quals = quals<VT>;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const & noexcept> final : invocable_using<Result(Args...) noexcept>, invoker<true, true, false, Result, Args...> {
			template<typename VT>
			using quals = const VT &;

			template<typename VT>
			using inv_quals = quals<VT>;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) &&> final : invocable_using<Result(Args...)>, invoker<false, false, true, Result, Args...> {
			template<typename VT>
			using quals = VT &&;

			template<typename VT>
			using inv_quals = quals<VT>;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const &&> final : invocable_using<Result(Args...)>, invoker<true, false, true, Result, Args...> {
			template<typename VT>
			using quals = const VT &&;

			template<typename VT>
			using inv_quals = quals<VT>;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) && noexcept> final : invocable_using<Result(Args...) noexcept>, invoker<false, true, true, Result, Args...> {
			template<typename VT>
			using quals = VT &&;

			template<typename VT>
			using inv_quals = quals<VT>;
		};

		template<typename Result, typename... Args>
		struct traits<Result(Args...) const && noexcept> final : invocable_using<Result(Args...) noexcept>, invoker<true, true, true, Result, Args...> {
			template<typename VT>
			using quals = const VT &&;

			template<typename VT>
			using inv_quals = quals<VT>;
		};


		template<typename Impl, typename Signature>
		struct function_call;

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...)> {
			auto operator()(Args... args) -> Result {
				auto & self{*static_cast<Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) const> {
			auto operator()(Args... args) const -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) noexcept> {
			auto operator()(Args... args) noexcept -> Result {
				auto & self{*static_cast<Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) const noexcept> {
			auto operator()(Args... args) const noexcept -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) &> {
			auto operator()(Args... args) & -> Result {
				auto & self{*static_cast<Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) const &> {
			auto operator()(Args... args) const & -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) & noexcept> {
			auto operator()(Args... args) & noexcept -> Result {
				auto & self{*static_cast<Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) const & noexcept> {
			auto operator()(Args... args) const & noexcept -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) &&> {
			auto operator()(Args... args) && -> Result {
				auto & self{*static_cast<Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) const &&> {
			auto operator()(Args... args) const && -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) && noexcept> {
			auto operator()(Args... args) && noexcept -> Result {
				auto & self{*static_cast<Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};

		template<typename Impl, typename Result, typename... Args>
		struct function_call<Impl, Result(Args...) const && noexcept> {
			auto operator()(Args... args) const && noexcept -> Result {
				auto & self{*static_cast<const Impl *>(this)};
				return self.vptr->dispatch(&self.storage, std::forward<Args>(args)...);
			}
		};


		template<typename...>
		struct is_move_only_function_specialization : std::false_type {};

		template<typename... Ts>
		struct is_move_only_function_specialization<move_only_function<Ts...>> : std::true_type {};

		template<typename... Ts>
		inline
		constexpr
		bool is_move_only_function_specialization_v{is_move_only_function_specialization<Ts...>::value};


		template<typename...>
		struct is_copyable_function_specialization : std::false_type {};

		template<typename... Ts>
		struct is_copyable_function_specialization<copyable_function<Ts...>> : std::true_type {};

		template<typename... Ts>
		inline
		constexpr
		bool is_copyable_function_specialization_v{is_copyable_function_specialization<Ts...>::value};


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
	class move_only_function<Signature> final : internal_function::function_call<move_only_function<Signature>, Signature> {
		using traits = internal_function::traits<Signature>;
		using vtable = internal_function::vtable<traits>;
		friend internal_function::function_call<move_only_function, Signature>;
		friend copyable_function<Signature>;

		template<typename... T>
		static
		constexpr
		bool is_invocable_using{traits::template is_invocable_using<T...>};

		template<typename VT>
		static
		constexpr
		bool is_callable_from{is_invocable_using<typename traits::template quals<VT>> && is_invocable_using<typename traits::template inv_quals<VT>>};

		const vtable * vptr;
		internal_function::storage_t storage;
	public:
		move_only_function() noexcept : vptr{vtable::init_empty()} {}
		move_only_function(std::nullptr_t) noexcept : move_only_function{} {}

		template<typename F, typename = std::enable_if_t<(!std::is_same_v<move_only_function, std::remove_cvref_t<F>> && !internal_function::is_copyable_function_specialization_v<std::remove_cvref_t<F>> && !internal_function::is_in_place_type_t_specialization_v<std::remove_cvref_t<F>> && is_callable_from<std::decay_t<F>>)>> //TODO: [C++20] replace with concepts/requires-clause
		move_only_function(F && func) {
			using VT = std::decay_t<F>;
			static_assert(std::is_constructible_v<VT, F>);
			if constexpr(std::is_function_v<std::remove_pointer_t<F>> || std::is_member_pointer_v<F> || internal_function::is_move_only_function_specialization_v<std::remove_cvref_t<F>>)
				if(!func) {
					vptr = vtable::init_empty();
					return;
				}
			vptr = vtable::template init_functor<false, VT>(storage, std::forward<F>(func));
		}

		template<typename T, typename... A>
		requires(std::is_constructible_v<std::decay_t<T>, A &&...> && is_callable_from<std::decay_t<T>>)
		explicit
		move_only_function(std::in_place_type_t<T>, A &&... args) {
			static_assert(std::is_same_v<T, std::decay_t<T>>);
			vptr = vtable::template init_functor<false, T>(storage, std::forward<A>(args)...);
		}

		template<typename T, typename U, typename... A>
		requires(std::is_constructible_v<std::decay_t<T>, std::initializer_list<U> &, A &&...> && is_callable_from<std::decay_t<T>>)
		explicit
		move_only_function(std::in_place_type_t<T>, std::initializer_list<U> ilist, A &&... args) {
			static_assert(std::is_same_v<T, std::decay_t<T>>);
			vptr = vtable::template init_functor<false, T>(storage, ilist, std::forward<A>(args)...);
		}

		move_only_function(const move_only_function &) =delete;

		move_only_function(move_only_function && other) noexcept { vtable::move_ctor(vptr, storage, other.vptr, other.storage); }

		auto operator=(const move_only_function &) -> move_only_function & =delete;

		auto operator=(move_only_function && other) noexcept -> move_only_function & {
			vtable::move_assign(vptr, storage, other.vptr, other.storage);
			return *this;
		}
		auto operator=(std::nullptr_t) noexcept -> move_only_function & {
			if(*this) move_only_function{}.swap(*this);
			return *this;
		}

		template<typename F>
		auto operator=(F && func) -> move_only_function & {
			move_only_function{std::forward<F>(func)}.swap(*this);
			return *this;
		}

		~move_only_function() noexcept { vptr->dtor(&storage); }

		using internal_function::function_call<move_only_function, Signature>::operator();

		explicit
		operator bool() const noexcept { return vptr->dispatch; }

		void swap(move_only_function & other) noexcept { vtable::swap(vptr, storage, other.vptr, other.storage); }
		friend
		void swap(move_only_function & lhs, move_only_function & rhs) noexcept { lhs.swap(rhs); }

		friend
		auto operator==(const move_only_function & self, std::nullptr_t) noexcept -> bool { return !self; }
	};


	template<typename Signature>
	class copyable_function<Signature> final : internal_function::function_call<copyable_function<Signature>, Signature> {
		using traits = internal_function::traits<Signature>;
		using vtable = internal_function::vtable<traits>;
		friend internal_function::function_call<copyable_function, Signature>;

		template<typename... T>
		static
		constexpr
		bool is_invocable_using{traits::template is_invocable_using<T...>};

		template<typename VT>
		static
		constexpr
		bool is_callable_from{is_invocable_using<typename traits::template quals<VT>> && is_invocable_using<typename traits::template inv_quals<VT>>};

		const vtable * vptr;
		internal_function::storage_t storage;
	public:
		copyable_function() noexcept : vptr{vtable::init_empty()} {}
		copyable_function(std::nullptr_t) noexcept : copyable_function{} {}

		template<typename F>
		requires(!std::is_same_v<copyable_function, std::remove_cvref_t<F>> && !internal_function::is_in_place_type_t_specialization_v<std::remove_cvref_t<F>> && is_callable_from<std::decay_t<F>>)
		copyable_function(F && func) {
			using VT = std::decay_t<F>;
			static_assert(std::is_constructible_v<VT, F>);
			if constexpr(std::is_function_v<std::remove_pointer_t<F>> || std::is_member_pointer_v<F> || internal_function::is_copyable_function_specialization_v<std::remove_cvref_t<F>>)
				if(!func) {
					vptr = vtable::init_empty();
					return;
				}
			vptr = vtable::template init_functor<true, VT>(storage, std::forward<F>(func));
		}

		template<typename T, typename... A>
		requires(std::is_constructible_v<std::decay_t<T>, A &&...> && is_callable_from<std::decay_t<T>>)
		explicit
		copyable_function(std::in_place_type_t<T>, A &&... args) {
			static_assert(std::is_same_v<T, std::decay_t<T>>);
			vptr = vtable::template init_functor<true, T>(storage, std::forward<A>(args)...);
		}

		template<typename T, typename U, typename... A>
		requires(std::is_constructible_v<std::decay_t<T>, std::initializer_list<U> &, A &&...> && is_callable_from<std::decay_t<T>>)
		explicit
		copyable_function(std::in_place_type_t<T>, std::initializer_list<U> ilist, A &&... args) {
			static_assert(std::is_same_v<T, std::decay_t<T>>);
			vptr = vtable::template init_functor<true, T>(storage, ilist, std::forward<A>(args)...);
		}

		copyable_function(const copyable_function & other) : vptr{other.vptr} { other.vptr->copy(&other.storage, &storage); }

		copyable_function(copyable_function && other) noexcept { vtable::move_ctor(vptr, storage, other.vptr, other.storage); }

		auto operator=(const copyable_function & other) -> copyable_function & {
			if(this != &other) {
				if(other.vptr->noexcept_copyable()) {
					vptr->dtor(&storage);
					other.vptr->copy(&other.storage, &storage);
				} else {
					internal_function::storage_t tmp;
					other.vptr->copy(&other.storage, &tmp);
					vptr->dtor(&storage);
					other.vptr->move(&tmp, &storage);
				}
				vptr = other.vptr;
			}
			return *this;
		}

		auto operator=(copyable_function && other) noexcept -> copyable_function & {
			vtable::move_assign(vptr, storage, other.vptr, other.storage);
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

		using internal_function::function_call<copyable_function, Signature>::operator();

		explicit
		operator bool() const noexcept { return vptr->dispatch; }

		explicit
		operator move_only_function<Signature>() const & {
			move_only_function<Signature> result;
			vptr->copy(&storage, &result.storage);
			result.vptr = vptr;
			return result;
		}
		operator move_only_function<Signature>() && noexcept {
			move_only_function<Signature> result;
			vptr->destructive_move(&storage, &result.storage);
			result.vptr = vptr;
			vptr = vtable::init_empty();
			return result;
		}

		void swap(copyable_function & other) noexcept { vtable::swap(vptr, storage, other.vptr, other.storage); }
		friend
		void swap(copyable_function & lhs, copyable_function & rhs) noexcept { lhs.swap(rhs); }

		friend
		auto operator==(const copyable_function & self, std::nullptr_t) noexcept -> bool { return !self; }
	};
}
