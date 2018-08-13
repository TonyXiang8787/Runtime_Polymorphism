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
		void(*copy_ctor) (void* this_, void const * that_);
		void(*copy_assign) (void* this_, void const * that_);
		// move
		void(*move_ctor) (void* this_, void * that_);
		void(*move_assign) (void* this_, void * that_);
	};
	template<class T>
	struct base_vtable_for {
		static void dtor(void* this_) {
			static_cast<T*>(this_)->~T();
		}
		static void copy_ctor(void* this_, void const* that_) {
			new (this_) T{ *static_cast<T const*>(that_) };
		}
		static void copy_assign(void* this_, void const* that_) {
			*static_cast<T*>(this_) = *static_cast<T const*>(that_);
		}
		static void move_ctor(void* this_, void* that_) {
			new (this_) T{ std::move(*static_cast<T*>(that_)) };
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
		vptr_->base_vtable.copy_ctor(&buffer_, &other.buffer_);
	}
	BasePolyClass& operator= (BasePolyClass const & other)
	{
		// same type
		if (vptr_ == other.vptr_)
			vptr_->base_vtable.copy_assign(&buffer_, &other.buffer_);
		else
		{
			// different type
			vptr_->base_vtable.dtor(&buffer_);  // destroy previous
			// assign new vptr_
			vptr_ = other.vptr_;
			// copy ctor
			vptr_->base_vtable.copy_ctor(&buffer_, &other.buffer_);
		}
		return *this;
	}
	// move
	BasePolyClass(BasePolyClass && other) :
		vptr_{ other.vptr_ } // vptr copy
	{
		// move content
		vptr_->base_vtable.move_ctor(&buffer_, &other.buffer_);
	}
	BasePolyClass& operator= (BasePolyClass && other)
	{
		// same type
		if (vptr_ == other.vptr_)
			vptr_->base_vtable.move_assign(&buffer_, &other.buffer_);
		else
		{
			// different type
			vptr_->base_vtable.dtor(&buffer_);  // destroy previous
			// assign new vptr_
			vptr_ = other.vptr_;
			// move ctor
			vptr_->base_vtable.move_ctor(&buffer_, &other.buffer_);
		}
		return *this;
	}
	
	// construct
	template<class T, class... Args>
	BasePolyClass(vtable_for<T>, Args&&... args) :
		vptr_{ &vtable_for<T>::value }
	{
		static_assert(sizeof(T) < sizeof(buffer_), "Buffer size exceeded!");
		new (&buffer_) T{ std::forward<Args>(args)... };
	}
	// destruct
	~BasePolyClass() {
		vptr_->base_vtable.dtor(&buffer_);
	}

	// getter
	void* ptr_buffer() { return &buffer_; }
	VTable const* vptr() { return vptr_; }
private:
	std::aligned_storage_t<buffer_size> buffer_;
	VTable const * vptr_;
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
			method_vtable.print_func(data_.ptr_buffer());
	}
private:
	BasePolyClassType data_;
};