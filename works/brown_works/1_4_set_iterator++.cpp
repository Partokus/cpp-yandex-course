#include <profile.h>
#include <test_runner.h>

#include <cassert>
#include <algorithm>
#include <iterator>
#include <map>
#include <vector>
#include <string>
#include <forward_list>
#include <iterator>
#include <deque>

using namespace std;

void TestAll();
void Profile();

struct Node
{
    Node(int v, Node *p)
        : value(v),
          parent(p)
    {
    }

    int value;
    Node *left = nullptr;
    Node *right = nullptr;
    Node *parent;
};

class NodeBuilder
{
public:
    Node *CreateRoot(int value)
    {
        nodes.emplace_back(value, nullptr);
        return &nodes.back();
    }

    Node *CreateLeftSon(Node *me, int value)
    {
        assert(me->left == nullptr);
        nodes.emplace_back(value, me);
        me->left = &nodes.back();
        return me->left;
    }

    Node *CreateRightSon(Node *me, int value)
    {
        assert(me->right == nullptr);
        nodes.emplace_back(value, me);
        me->right = &nodes.back();
        return me->right;
    }

private:
    deque<Node> nodes;
};

Node *Next(Node *me)
{
    if (me->right != nullptr and me->right->value > me->value)
    {
        // правый узел не пустой, ищем теперь
        // от него самый левый узел
        Node *result = me->right;
        while (result->left != nullptr)
        {
            result = result->left;
        }
        return result;
    }

    // если узел является левым узлом родителя, то мы
    // точно знаем, то родитель является следующим
    // по возрастанию
    if (me->parent != nullptr and me->parent->left == me)
    {
        return me->parent;
    }

    // пока узел является правым узлом родителя,
    // двигаемся вверх по дереву, пока не найдём
    // конец дерева или узел не станет являться
    // левым узлом для родителя
    while (me->parent && me == me->parent->right)
    {
        me = me->parent;
    }
    return me->parent;
}

int main()
{
    TestAll();
    Profile();
    return 0;
}

void Test1()
{
    NodeBuilder nb;

    Node *root = nb.CreateRoot(50);
    ASSERT_EQUAL(root->value, 50);

    Node *l = nb.CreateLeftSon(root, 2);
    Node *min = nb.CreateLeftSon(l, 1);
    Node *r = nb.CreateRightSon(l, 4);
    ASSERT_EQUAL(min->value, 1);
    ASSERT_EQUAL(r->parent->value, 2);

    nb.CreateLeftSon(r, 3);
    nb.CreateRightSon(r, 5);

    r = nb.CreateRightSon(root, 100);
    l = nb.CreateLeftSon(r, 90);
    nb.CreateRightSon(r, 101);

    nb.CreateLeftSon(l, 89);
    r = nb.CreateRightSon(l, 91);

    ASSERT_EQUAL(Next(l)->value, 91);
    ASSERT_EQUAL(Next(root)->value, 89);
    ASSERT_EQUAL(Next(min)->value, 2);
    ASSERT_EQUAL(Next(r)->value, 100);

    while (min)
    {
        cout << min->value << '\n';
        min = Next(min);
    }
}

void TestRootOnly()
{
    NodeBuilder nb;
    Node *root = nb.CreateRoot(42);
    ASSERT(Next(root) == nullptr);
};

void TestAll()
{
    TestRunner tr{};
    RUN_TEST(tr, Test1);
    RUN_TEST(tr, TestRootOnly);
}

void Profile()
{
}
