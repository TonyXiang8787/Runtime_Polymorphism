// Runtime_Polymorphism.cpp : Defines the entry point for the console application.
//
#include <vector>
#include <iostream>
#include <memory>
#include "poly_class.hpp"


class C1 {
public:
	explicit C1(int x) : x_{ x } {}

	void print_func() {
		std::cout << "C1 class: " << x_ << '\n';
	};

private:
	int x_;
};

class C2 {
public:
	explicit C2(double x) : x_{ x } {}


	void print_func() {
		std::cout << "C2 class: " << x_ << '\n';
	};

private:
	double x_;
};

class C3 {
public:
	C3(int begin, int end) {
		for (int i = begin; i < end; ++i)
			x_.push_back(i);
	}

	void print_func() {
		std::cout << "C3 class: ";
		for (auto & i : x_)
		{
			std::cout << i << ", ";
		}
		std::cout << '\n';
	};

private:
	std::vector<int> x_;
};


int main()
{
	std::vector<PolyClass> vec;
	vec.reserve(5);
	vec.emplace_back(PolyClass::type_tag_v<C1>, 10);
	vec.emplace_back(PolyClass::type_tag_v<C2>, 100.0);
	vec.emplace_back(PolyClass::type_tag_v<C3>, 5, 9);

	for (auto & ele : vec)
		ele.print_func();

	//std::vector<PolyClass> vec2{ vec };
	PolyClass p1{ vec[2] };
	p1.print_func();
	PolyClass p2{ std::move(p1) };
	p1.print_func();

	p2 = vec[1];
	p2 = vec[0];
	p2 = std::move(vec[2]);
	vec[2].print_func();
	p2.print_func();

	PolyClass p3{ vec[1] };
	PolyClass p4{ std::move(p3) };

    return 0;
}

