#include "array.h"

#include "catch.h"

struct NonPODObject
{
    NonPODObject() : m_X(0) {}
    NonPODObject(int x) : m_X(x) {}
    virtual ~NonPODObject() {}
    operator int() const { return m_X; }
    int m_X;
};

TEST_CASE("Array Construction")
{
    // POD Type
    {
        // Default construct
        Array<int> array;
        REQUIRE(array.Size() == 0);
        REQUIRE(array.Capacity() == 0);
    }
    {
        // Construct with capacity
        Array<int> array(10);
        REQUIRE(array.Size() == 0);
        REQUIRE(array.Capacity() == 10);
    }
    {
        // Construct with initializer list
        Array<int> array{5, 10, 25};
        REQUIRE(array.Size() == 3);
        REQUIRE(array.Capacity() == 3);
        REQUIRE(array[0] == 5);
        REQUIRE(array[1] == 10);
        REQUIRE(array[2] == 25);
    }
    {
        Array<int> array1{5, 10, 25};
        Array<int> array2(array1);
        REQUIRE(array2.Size() == 3);
        REQUIRE(array2.Capacity() == 3);
        REQUIRE(array2[0] == 5);
        REQUIRE(array2[1] == 10);
        REQUIRE(array2[2] == 25);
    }
    {
        Array<int> array1{5, 10, 25};
        Array<int> array2(std::move(array1));
        REQUIRE(array2.Size() == 3);
        REQUIRE(array2.Capacity() == 3);
        REQUIRE(array2[0] == 5);
        REQUIRE(array2[1] == 10);
        REQUIRE(array2[2] == 25);
        REQUIRE(array1.Size() == 0);
        REQUIRE(array1.Capacity() == 0);
    }
    // Non POD Type
    {
        // Default construct
        Array<NonPODObject> array;
        REQUIRE(array.Size() == 0);
        REQUIRE(array.Capacity() == 0);
    }
    {
        // Construct with capacity
        Array<NonPODObject> array(10);
        REQUIRE(array.Size() == 0);
        REQUIRE(array.Capacity() == 10);
    }
    {
        // Construct with initializer list
        Array<NonPODObject> array{5, 10, 25};
        REQUIRE(array.Size() == 3);
        REQUIRE(array.Capacity() == 3);
        REQUIRE(array[0] == 5);
        REQUIRE(array[1] == 10);
        REQUIRE(array[2] == 25);
    }
    {
        Array<NonPODObject> array1{5, 10, 25};
        Array<NonPODObject> array2(array1);
        REQUIRE(array2.Size() == 3);
        REQUIRE(array2.Capacity() == 3);
        REQUIRE(array2[0] == 5);
        REQUIRE(array2[1] == 10);
        REQUIRE(array2[2] == 25);
    }
    {
        Array<NonPODObject> array1{5, 10, 25};
        Array<NonPODObject> array2(std::move(array1));
        REQUIRE(array2.Size() == 3);
        REQUIRE(array2.Capacity() == 3);
        REQUIRE(array2[0] == 5);
        REQUIRE(array2[1] == 10);
        REQUIRE(array2[2] == 25);
        REQUIRE(array1.Size() == 0);
        REQUIRE(array1.Capacity() == 0);
    }
}

TEST_CASE("Array assignment")
{
    Array<int> array{5, 4, 3};
    SECTION("By copy")
    {    
        {
            Array<int> array2;
            array2 = array;
            REQUIRE(array2.Size() == array.Size());
            REQUIRE(array.Capacity() == array2.Capacity());
            REQUIRE(array[0] == array2[0]);
            REQUIRE(array[1] == array2[1]);
            REQUIRE(array[2] == array2[2]);
        }
        {
            Array<int> array2{1};
            array2 = array;
            REQUIRE(array2.Size() == array.Size());
            REQUIRE(array.Capacity() == array2.Capacity());
            REQUIRE(array[0] == array2[0]);
            REQUIRE(array[1] == array2[1]);
            REQUIRE(array[2] == array2[2]);
        }
    }
    SECTION("Move Empty")
    {
        Array<int> array2;
        array2 = std::move(array);
        REQUIRE(array2.Size() == 3);
        REQUIRE(array2[0] == 5);
        REQUIRE(array2[1] == 4);
        REQUIRE(array2[2] == 3);
    }
    SECTION("Move Initialized")
    {
        Array<int> array2{9, 8};
        array2 = std::move(array);
        REQUIRE(array2.Size() == 3);
        REQUIRE(array2[0] == 5);
        REQUIRE(array2[1] == 4);
        REQUIRE(array2[2] == 3);
    }
}

TEST_CASE("Array Push")
{
    Array<int> array;    
    // Non-POD
    {
        SECTION("Single Value")
        {
            array.Push(5);
            REQUIRE(array.Size() == 1);
            REQUIRE(array[0] == 5);
            array.Push(6);
            REQUIRE(array.Size() == 2);
            REQUIRE(array[0] == 5);
            REQUIRE(array[1] == 6);            
        }
        SECTION("Initializer")
        {
            array.Push({5, 6});
            REQUIRE(array.Size() == 2);
            REQUIRE(array[0] == 5);
            REQUIRE(array[1] == 6);
        }
    }
}

TEST_CASE("Array Pop()")
{
    Array<int> array{5,4,3};
    array.Pop();
    REQUIRE(!array.Empty());
    REQUIRE(array.Size() == 2);
    REQUIRE(array[0] == 5);
    REQUIRE(array[1] == 4);
    array.Pop();
    REQUIRE(array.Size() == 1);
    REQUIRE(array[0] == 5);
    array.Pop();
    REQUIRE(array.Size() == 0);
    REQUIRE(array.Empty());
}

TEST_CASE("Iterators")
{
    {
        Array<int> array{5, 4, 3, 2};
        REQUIRE(array.end() - array.begin() == 4);
        REQUIRE(*array.begin() == 5);
        REQUIRE(*(array.begin() + 1) == 4);
    }
    {
        const Array<int> array{5, 4, 3, 2};
        REQUIRE(array.end() - array.begin() == 4);
        REQUIRE(*array.begin() == 5);
        REQUIRE(*(array.begin() + 1) == 4);
    }            
}

TEST_CASE("Reserve")
{
    Array<int> array;
    REQUIRE(array.Capacity() == 0);
    REQUIRE(array.Size() == 0);
    array.Reserve(10);
    REQUIRE(array.Size() == 0);
    REQUIRE(array.Capacity() == 10);
}

TEST_CASE("Array Clear()")
{
    Array<int> array{5, 4, 2};
    array.Clear();
    REQUIRE(array.Size() == 0);
    REQUIRE(array.Capacity() == 3);
}

TEST_CASE("Array Resize")
{
    {
        Array<int> array{5, 4};
        SECTION("Resize smaller")
        {
            array.Resize(1);
            REQUIRE(array.Size() == 1);
            REQUIRE(array[0] == 5);
        }
        SECTION("Resize larger")
        {
            array.Resize(3);
            REQUIRE(array.Size() == 3);
            REQUIRE(array[0] == 5);
            REQUIRE(array[1] == 4);
        }
    }
    {
        Array<NonPODObject> array{5, 4};
        SECTION("Resize smaller")
        {
            array.Resize(1);
            REQUIRE(array.Size() == 1);
            REQUIRE(array[0] == 5);
        }
        SECTION("Resize larger")
        {
            array.Resize(3);
            REQUIRE(array.Size() == 3);
            REQUIRE(array[0] == 5);
            REQUIRE(array[1] == 4);
        }        
    }
}

TEST_CASE("Grow")
{
    Array<int> array{5, 4, 3};
    array.Grow() = 12;
    REQUIRE(array[3] == 12);
    REQUIRE(array.Last() == 12);
}

