/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Trie.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hicham <hibenouk@1337.ma>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/29 16:45:56 by hicham            #+#    #+#             */
/*   Updated: 2024/10/17 12:32:27 by hibenouk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Trie.hpp"
#include <cstring>
#include "ParserException.hpp"

Trie::TrieNode::TrieNode() : isEnd(false)
{
	memset(children, 0, sizeof(children));
}

Trie::TrieNode::~TrieNode() {}

Trie::Trie()
{
	root = new TrieNode();
}

void Trie::deleteNode()
{
	this->_deleteNode(root); // fuck destructor
}

void Trie::_deleteNode(TrieNode *node)
{
	if (node == NULL)
		return;
	for (size_t i = 0; i < 128; i++)
	{
		if (node->children[i] != NULL)
		{
			_deleteNode(node->children[i]);
			node->children[i] = NULL;
		}
	}
	delete node;
}

Trie::~Trie() {}

bool Trie::insert(Location &location)
{
	const std::string &path = location.getPath();
	TrieNode *currNode = root;
	int idx;
	for (size_t i = 0; i < path.size(); i++)
	{
		idx = path[i];
		if (idx < 0 || idx > 128)
			throw ParserException("InValide char in path " + path);
		if (currNode->children[idx] == NULL)
			currNode->children[idx] = new TrieNode();
		currNode = currNode->children[idx];
	}
	if (currNode->isEnd)
		throw ParserException("Duplicate location route at: " + path);
	currNode->isEnd = true;
	currNode->location = location;
	this->locations.push_back(&currNode->location);
	return (true);
}

Location *Trie::findPath(const std::string &route)
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
	return (location);
}

void Trie::init(const GlobalConfig &conf)
{
	for (size_t i = 0; i < this->locations.size(); i++)
		this->locations[i]->globalConfig = conf;
}
