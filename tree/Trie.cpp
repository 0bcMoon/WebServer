#include "Trie.hpp"
#include "ParserException.hpp"

Trie::TrieNode::TrieNode() : isEnd(false)
{
	std::memset(children, 0, sizeof(children));
}

Trie::TrieNode::~TrieNode()
{
	// delete location; handel memory leak
}

Trie::Trie()
{
	root = new TrieNode();
}
Trie::~Trie()
{
	// delete root; handel memory leak
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
	currNode->location = location;
}

Location *Trie::findPath(std::string &route)
{
	TrieNode *currNode = root;
	Location location;
	int idx;
	for (size_t i = 0; i < route.size(); i++)
	{
		idx = route[i];
		if (currNode->children[idx] == NULL)
			break;
		if (currNode->children[idx]->isEnd)
			location = currNode->children[idx]->location;
		currNode = currNode->children[idx];
	}
	return NULL; // todo may return tieNode cuz is pointer
}
