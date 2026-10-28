#include <doctest/doctest.h>

#include <yqlora/core/Utf8Path.hpp>
#include <yqlora/scene/MeshRenderer.hpp>
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/NodeTree.hpp>

using namespace yqlora;

TEST_CASE("NodeTree: find a node and check the branch")
{
    Node3D root;
    auto& child = root.addChild<Node3D>();
    auto& grandchild = child.addChild<Node3D>();
    auto& other = root.addChild<Node3D>();

    CHECK(findNode(root, grandchild.id()) == &grandchild);
    CHECK(findNode(root, root.id()) == &root);
    CHECK(findNode(child, other.id()) == nullptr);

    CHECK(isInBranch(root, grandchild));
    CHECK(isInBranch(child, child));
    CHECK_FALSE(isInBranch(child, other));
    CHECK_FALSE(isInBranch(grandchild, child));
}

TEST_CASE("NodeTree: component place in the list")
{
    Node3D node;
    auto& renderer = node.addComponent<MeshRenderer>();
    const Node3D stranger;

    CHECK(componentIndex(node, renderer) == 1);
    CHECK(componentAt(node, 1) == &renderer);
    CHECK(componentAt(node, 2) == nullptr);
    CHECK_FALSE(componentIndex(stranger, renderer));
}

TEST_CASE("Utf8Path: Cyrillic survives the round trip")
{
    const std::string text = "Проекты/Игра";
    const std::filesystem::path path = fromUtf8(text);
    CHECK(path.filename().u8string() == u8"Игра");
    CHECK(toGenericUtf8(path) == text);
    CHECK(fromUtf8(toUtf8(path)) == path);
}
