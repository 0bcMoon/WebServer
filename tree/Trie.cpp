#include "Trie.hpp"
#include "ParserException.hpp"

Trie::TrieNode::TrieNode() : isEnd(false)
{
	std::memset(children, 0, sizeof(children));
}

Trie::TrieNode::~TrieNode()
{

}

Trie::Trie()
{
	std::cout << "Trie constructor" << std::endl;
	root = new TrieNode();
	std::cout << root << std::endl;
}

void Trie::deleteNode(TrieNode *node)
{
	if (node == NULL)
		return;

	for (size_t i = 0; i < 128; i++)
	{
		if (node->children[i] != NULL)
		{
			deleteNode(node->children[i]);
			node->children[i] = NULL;
		}
	}
	delete node;
}

Trie::~Trie()
{
	for (size_t i = 0; i < 128; i++)
	{
		if (root->children[i] != NULL)
		{
			deleteNode(root->children[i]);
			root->children[i] = NULL;
		}
	}
}

void Trie::insert(Location &location)
{
	std::string &path = location.getPath();
	TrieNode *currNode = root;
	int idx;
	for (size_t i = 0; i < path.size(); i++)
	{
		idx = path[i];
		if (idx < 0 || idx > 128)
			throw ParserException("Invalid character in path");
		if (currNode->children[idx] == NULL)
			currNode->children[idx] = new TrieNode();
		currNode = currNode->children[idx];
	}
	if (currNode->isEnd)
		throw ParserException("Location already exists");
	currNode->isEnd = true;
	currNode->location = location;
}


Location *Trie::findPath(std::string &route)
{
	TrieNode *currNode = root;
	Location *location = NULL;
	int idx;
	for (size_t i = 0; i < route.size(); i++)
	{
		idx = route[i];
		if (idx < 0 || idx > 128)
			return location; // TODO this this work
		if (currNode->children[idx] == NULL)
			break;
		if (currNode->children[idx]->isEnd)
			location = &currNode->children[idx]->location;
		currNode = currNode->children[idx];
	}
	return location; // todo may return tieNode cuz is pointer
}
