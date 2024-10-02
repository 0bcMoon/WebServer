#ifndef Trie_H
#define Trie_H
#include <Location.hpp>
#include <string>
class Trie
{
  private:
	struct TrieNode
	{
		Location	location;
		bool		isEnd;
		TrieNode	*children[128];

		TrieNode();
		~TrieNode();
	};

	TrieNode *root;
  public:
	Trie();
	~Trie();

	void deleteNode();
	void _deleteNode(TrieNode *node);
	bool insert(Location &location);
	Location *findPath(std::string &route);
};

#endif
