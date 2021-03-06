#pragma once
// poly class

#include <cstddef>
#include <memory>


template<size_t buffer_size, 
	class MethodVTable, 
	template<class> class method_vtable_for>
class BasePolyClass {
private:
	struct BaseVTable {
		// destructor
		void(*dtor) (void* this_);
		// copy
		void* (*copy_ctor) (void* this_, void const * that_);
		void(*copy_assign) (void* this_, void const * that_);
		// move
		void* (*move_ctor) (void* this_, void * that_);
		void(*move_assign) (void* this_, void * that_);
	};
	template<class T>
	struct base_vtable_for {
		static bool constexpr on_heap =
			(sizeof(T) > buffer_size);
		static void dtor(void* this_) {
			if constexpr (on_heap)
				delete static_cast<T*>(this_);
			else
				static_cast<T*>(this_)->~T();
		}
		static void* copy_ctor(void* this_, void const* that_) {
			if constexpr (on_heap)
				return new T{ *static_cast<T const*>(that_) };
			else
				return new (this_) T{ *static_cast<T const*>(that_) };
		}
		static void copy_assign(void* this_, void const* that_) {
			*static_cast<T*>(this_) = *static_cast<T const*>(that_);
		}
		static void* move_ctor(void* this_, void* that_) {
			if constexpr (on_heap)
				return new T{ std::move(*static_cast<T*>(that_)) };
			else
				return new (this_) T{ std::move(*static_cast<T*>(that_)) };
		}
		static void move_assign(void* this_, void* that_) {
			*static_cast<T*>(this_) = std::move(*static_cast<T*>(that_));
		}
		static BaseVTable constexpr value = {
			&dtor,
			&copy_ctor,
			&copy_assign,
			&move_ctor,
			&move_assign
		};
	};

	struct VTable {
		BaseVTable base_vtable;
		MethodVTable method_vtable;
	};

	template<class T>
	struct vtable_for {
		static VTable constexpr value = {
			base_vtable_for<T>::value,
			method_vtable_for<T>::value
		};
	};

public:
	template<class T>
	static vtable_for<T> constexpr type_tag_v{};

	// copy
	BasePolyClass(BasePolyClass const & other) :
		vptr_{ other.vptr_ } // vptr copy
	{
		// copy content
		ptr_ = vptr_->base_vtable.copy_ctor(&buffer_, other.ptr_);
	}
	BasePolyClass& operator= (BasePolyClass const & other)
	{
		// same type
		if (vptr_ == other.vptr_)
			vptr_->base_vtable.copy_assign(ptr_, other.ptr_);
		else
		{
			// different type
			vptr_->base_vtable.dtor(ptr_);  // destroy previous
			// assign new vptr_
			vptr_ = other.vptr_;
			// copy ctor
			ptr_ = vptr_->base_vtable.copy_ctor(&buffer_, other.ptr_);
		}
		return *this;
	}
	// move
	BasePolyClass(BasePolyClass && other) :
		vptr_{ other.vptr_ } // vptr copy
	{
		// move content
		ptr_ = vptr_->base_vtable.move_ctor(&buffer_, other.ptr_);
	}
	BasePolyClass& operator= (BasePolyClass && other)
	{
		// same type
		if (vptr_ == other.vptr_)
			vptr_->base_vtable.move_assign(ptr_, other.ptr_);
		else
		{
			// different type
			vptr_->base_vtable.dtor(ptr_);  // destroy previous
			// assign new vptr_
			vptr_ = other.vptr_;
			// move ctor
			ptr_ = vptr_->base_vtable.move_ctor(&buffer_, other.ptr_);
		}
		return *this;
	}
	
	// construct
	template<class T, class... Args>
	BasePolyClass(vtable_for<T>, Args&&... args) :
		vptr_{ &vtable_for<T>::value }
	{
		if constexpr(base_vtable_for<T>::on_heap)
			ptr_ = new T{ std::forward<Args>(args)... };
		else
			ptr_ = new (&buffer_) T{ std::forward<Args>(args)... };
	}
	// destruct
	~BasePolyClass() {
		vptr_->base_vtable.dtor(ptr_);
	}

	// getter
	void* ptr() { return ptr_; }
	VTable const* vptr() { return vptr_; }
private:
	VTable const * vptr_;
	void * ptr_;
	std::aligned_storage_t<buffer_size> buffer_;
};

class PolyClass {
private:
	static size_t constexpr poly_buffer_size = 64;

	struct MethodVTable {
		void(*print_func) (void* this_);
	};
	template<class T>
	struct method_vtable_for {
		static void print_func(void* this_) {
			static_cast<T*>(this_)->print_func();
		}
		static MethodVTable constexpr value = {
			&print_func
		};
	};

	using BasePolyClassType =
		BasePolyClass<poly_buffer_size, MethodVTable, method_vtable_for>;

public:
	template<class T>
	static method_vtable_for<T> constexpr type_tag_v{};

	// constructor
	template<class T, class... Args>
	PolyClass(method_vtable_for<T>, Args&&... args) :
		data_{
		BasePolyClassType::type_tag_v<T>,
		std::forward<Args>(args)... }
	{
	}

	void print_func() {
		data_.vptr()->
			method_vtable.print_func(data_.ptr());
	}
private:
	BasePolyClassType data_;
};