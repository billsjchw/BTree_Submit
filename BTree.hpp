#include "utility.hpp"
#include <functional>
#include <cstddef>
#include <fstream>
#include "exception.hpp"
namespace sjtu {
template <class Key, class Value, class Compare = std::less<Key> >
class BTree {
private:
	static const int M = 8192 / (sizeof(Key) + sizeof(size_t));
	static const int L = 8192 / (sizeof(Key) + sizeof(Value));
	static const size_t HEAD = 0, TAIL = 1;
private:
	struct Node {
		int len;
		bool leaf;
		Key keys[M];
		size_t childs[M + 1];
		Node(int len = 0, bool leaf = false):
			len(len), leaf(leaf) {}
	};
	struct Block {
		int len;
		Key keys[L + 1];
		Value values[L + 1];
		size_t prev, next;
		Block(int len = 0, size_t prev = 0, size_t next = 0):
			len(len), prev(prev), next(next) {}
	};
private:
	// Your private members go here
	size_t root, len;
	std::fstream nodes, blocks;
	Compare cmp;
public:
	typedef pair<const Key, Value> value_type;

	class const_iterator;
	class iterator {
	private:
		// Your private members go here
	public:
		bool modify(const Value& value){

		}
		iterator() {
		// TODO Default Constructor
		}
		iterator(const iterator& other) {
		// TODO Copy Constructor
		}
		// Return a new iterator which points to the n-next elements
		iterator operator++(int) {
		// Todo iterator++
		}
		iterator& operator++() {
		// Todo ++iterator
		}
		iterator operator--(int) {
		// Todo iterator--
		}
		iterator& operator--() {
		// Todo --iterator
		}
		// Overloaded of operator '==' and '!='
		// Check whether the iterators are same
		bool operator==(const iterator& rhs) const {
		// Todo operator ==
		}
		bool operator==(const const_iterator& rhs) const {
		// Todo operator ==
		}
		bool operator!=(const iterator& rhs) const {
		// Todo operator !=
		}
		bool operator!=(const const_iterator& rhs) const {
		// Todo operator !=
		}
	};
	class const_iterator {
		// it should has similar member method as iterator.
		//  and it should be able to construct from an iterator.
	private:
		// Your private members go here
	public:
		const_iterator() {
		// TODO
		}
		const_iterator(const const_iterator& other) {
		// TODO
		}
		const_iterator(const iterator& other) {
		// TODO
		}
		// And other methods in iterator, please fill by yourself.
	};
private:
	void initialize() {
		root = 0; len = 0;
		std::ofstream info("info.dat", std::ios::binary);
		info.write((char *) &root, sizeof(root));
		info.write((char *) &len, sizeof(len));
		info.close();
		Node rt = Node(1, true); rt.childs[0] = 2;
		std::ofstream nodes("nodes.dat", std::ios::binary);
		nodes.write((char *) &rt, sizeof(rt));
		nodes.close();
		Block head(0, 0, 2), tail(0, 2, 0), first(0, HEAD, TAIL);
		std::ofstream blocks("blocks.dat", std::ios::binary);
		blocks.write((char *) &head, sizeof(head));
		blocks.write((char *) &tail, sizeof(tail));
		blocks.write((char *) &first, sizeof(first));
		blocks.close();
	}
	size_t ntop() {
		nodes.seekg(0, std::ios::end);
		return nodes.tellg() / sizeof(Node);
	}
	size_t btop() {
		blocks.seekg(0, std::ios::end);
		return blocks.tellg() / sizeof(Block);
	}
	Node get_node(size_t pos) {
		Node ret;
		nodes.seekg(pos * sizeof(Node), std::ios::beg);
		nodes.read((char *) &ret, sizeof(ret));
		return ret;
	}
	Block get_block(size_t pos) {
		Block ret;
		blocks.seekg(pos * sizeof(Block), std::ios::beg);
		blocks.read((char *) &ret, sizeof(ret));
		return ret;
	}
	void put_node(const Node & node, size_t pos) {
		nodes.seekp(pos * sizeof(Node), std::ios::beg);
		nodes.write((char *) &node, sizeof(node));
	}
	void put_block(const Block & block, size_t pos) {
		blocks.seekp(pos * sizeof(Block), std::ios::beg);
		blocks.write((char *) &block, sizeof(block));
	}
	Value at(const Key & key, size_t rt) {
		Node node = get_node(rt);
		int k;
		for (k = 0; k < node.len - 1 && !cmp(key, node.keys[k]); ++k);
		if (node.leaf) {
			Block block = get_block(node.childs[k]);
			for (int i = 0; i < block.len; ++i)
				if (!cmp(block.keys[i], key) && !cmp(key, block.keys[i])) 
					return block.values[i];
		} else
			return at(key, node.childs[k]);
	}
	bool split_block(size_t rt, int k) {
		Node node = get_node(rt);
		Block p = get_block(node.childs[k]);
		Block q(L + 1 - (L + 1) / 2, node.childs[k], p.next);
		Block r = get_block(p.next);
		for (int i = 0; i < L + 1 - (L + 1) / 2; ++i) {
			q.keys[i] = p.keys[i + (L + 1) / 2];
			q.values[i] = p.values[i + (L + 1) / 2];
		}
		p.len = (L + 1) / 2;
		p.next = r.prev = btop();
		for (int i = node.len - 1; i > k; --i) {
			node.keys[i] = node.keys[i - 1];
			node.childs[i + 1] = node.childs[i];
		}
		node.keys[k] = q.keys[0];
		node.childs[k + 1] = btop();
		++node.len;
		put_block(p, node.childs[k]);
		put_block(q, btop());
		put_block(r, q.next);
		put_node(node, rt);
		return node.len == M + 1;
	}
	bool split_node(size_t rt, int k) {
		Node node = get_node(rt);
		Node p = get_node(node.childs[k]);
		Node q(M + 1 - (M + 1) / 2, p.leaf);
		for (int i = 0; i < M - (M + 1) / 2; ++i) {
			q.keys[i] = p.keys[i + (M + 1) / 2];
			q.childs[i] = p.childs[i + (M + 1) / 2];
		}
		q.childs[M - (M + 1) / 2] = p.childs[M];
		p.len = (M + 1) / 2;
		for (int i = node.len - 1; i > k; --i) {
			node.keys[i] = node.keys[i - 1];
			node.childs[i + 1] = node.childs[i];
		}
		node.keys[k] = p.keys[(M + 1) / 2 - 1];
		node.childs[k + 1] = ntop();
		++node.len;
		put_node(p, node.childs[k]);
		put_node(q, ntop());
		put_node(node, rt);
		return node.len == M + 1;
	}
	pair<iterator, OperationResult> insert(const Key & key, const Value & value, size_t rt, bool & large) {
		large = false;
		Node node = get_node(rt);
		int k;
		for (k = 0; k < node.len - 1 && !cmp(key, node.keys[k]); ++k);
		if (k && !cmp(node.keys[k - 1], key))
			return pair<iterator, OperationResult>(iterator(), Fail);
		if (node.leaf) {
			Block block = get_block(node.childs[k]);
			int i;
			for (i = 0; i < block.len && cmp(block.keys[i], key); ++i);
			if (i != block.len && !cmp(key, block.keys[i]))
				return pair<iterator, OperationResult>(iterator(), Fail);
			for (int j = block.len - 1; j >= i; --j) {
				block.keys[j + 1] = block.keys[j];
				block.values[j + 1] = block.values[j];
			}
			block.keys[i] = key;
			block.values[i] = value;
			++block.len;
			put_block(block, node.childs[k]);
			if (block.len == L + 1) 
				large = split_block(rt, k);
		} else {
			bool flag;
			insert(key, value, node.childs[k], flag);
			if (flag)
				large = split_node(rt, k);
		}
		return pair<iterator, OperationResult>(iterator(), Success);
	}
	void adopt_from_left_node(size_t rt, int k) {
		Node node = get_node(rt);
		Node p = get_node(node.childs[k - 1]);
		Node q = get_node(node.childs[k]);
		for (int i = q.len - 1; i > 0; --i) {
			q.childs[i + 1] = q.childs[i];
			q.keys[i] = q.keys[i - 1];
		}
		q.childs[1] = q.childs[0];
		q.childs[0] = p.childs[p.len - 1];
		q.keys[0] = node.keys[k - 1];
		--p.len; ++q.len;
		node.keys[k - 1] = p.keys[p.len - 2];
		put_node(node, rt);
		put_node(p, node.childs[k - 1]);
		put_node(q, node.childs[k]);
	}
	void adopt_from_right_node(size_t rt, int k) {
		Node node = get_node(rt);
		Node p = get_node(node.childs[k]);
		Node q = get_node(node.childs[k + 1]);
		p.childs[p.len] = q.childs[0];
		p.keys[p.len - 1] = node.keys[k];
		node.keys[k] = q.keys[0];
		for (int i = 0; i < q.len - 2; ++i) {
			q.childs[i] = q.childs[i + 1];
			q.keys[i] = q.keys[i + 1];
		}
		q.childs[q.len - 2] = q.childs[q.len - 1];
		++p.len; --q.len;
		put_node(node, rt);
		put_node(p, node.childs[k]);
		put_node(q, node.childs[k + 1]);
	}
	bool merge_nodes(size_t rt, int k) {
		Node node = get_node(rt);
		Node p = get_node(node.childs[k]);
		Node q = get_node(node.childs[k + 1]);
		for (int i = 0; i < q.len - 1; ++i) {
			p.childs[p.len + i] = q.childs[i];
			p.keys[p.len + i] = q.keys[i];
		}
		p.childs[p.len + q.len - 1] = q.childs[q.len - 1];
		p.keys[p.len - 1] = node.keys[k];
		p.len += q.len;
		for (int i = k + 1; i < node.len - 1; ++i) {
			node.childs[i] = node.childs[i + 1];
			node.keys[i - 1] = node.keys[i];
		}
		--node.len;
		put_node(node, rt);
		put_node(p, node.childs[k]);
		return node.len < (M + 1) / 2; 
	}
	void adopt_from_left_block(size_t rt, int k) {
		Node node = get_node(rt);
		Block p = get_block(node.childs[k - 1]);
		Block q = get_block(node.childs[k]);
		for (int i = q.len - 1; i >= 0; --i) {
			q.keys[i + 1] = q.keys[i];
			q.values[i + 1] = q.values[i];
		}
		q.keys[0] = p.keys[p.len - 1];
		q.values[0] = p.values[p.len - 1];
		--p.len; ++q.len;
		node.keys[k - 1] = q.keys[0];
		put_node(node, rt);
		put_block(p, node.childs[k - 1]);
		put_block(q, node.childs[k]);
	}
	void adopt_from_right_block(size_t rt, int k) {
		Node node = get_node(rt);
		Block p = get_block(node.childs[k]);
		Block q = get_block(node.childs[k + 1]);
		p.keys[p.len] = q.keys[0];
		p.values[p.len] = q.values[0];
		for (int i = 0; i < q.len - 1; ++i) {
			q.keys[i] = q.keys[i + 1];
			q.values[i] = q.values[i + 1];
		}
		++p.len; --q.len;
		node.keys[k] = q.keys[0];
		put_node(node,rt);
		put_block(p, node.childs[k]);
		put_block(q, node.childs[k + 1]);
	}
	bool merge_blocks(size_t rt, int k) {
		Node node = get_node(rt);
		Block p = get_block(node.childs[k]);
		Block q = get_block(node.childs[k + 1]);
		Block r = get_block(q.next);
		for (int i = 0; i < q.len; ++i) {
			p.keys[p.len + i] = q.keys[i];
			p.values[p.len + i] = q.values[i];
		}
		p.len += q.len;
		p.next = q.next;
		r.prev = node.childs[k];
		for (int i = k + 1; i < node.len - 1; ++i) {
			node.childs[i] = node.childs[i + 1];
			node.keys[i - 1] = node.keys[i];
		}
		--node.len;
		put_node(node,rt);
		put_block(p, node.childs[k]);
		put_block(r, p.next);
		return node.len < (M + 1) / 2;
	}
	bool fix_small_block(size_t rt, int k) {
		bool ret = false;
		Node node = get_node(rt);
		if (k != node.len - 1) {
			Block block = get_block(node.childs[k + 1]);
			if (block.len == (L + 1) / 2)
				ret = merge_blocks(rt, k);
			else
				adopt_from_right_block(rt, k);
		} else if (k) {
			Block block = get_block(node.childs[k - 1]);
			if (block.len == (L + 1) / 2)
				ret = merge_blocks(rt, k - 1);
			else
				adopt_from_left_block(rt, k);
		}
		return ret;
	}
	bool fix_small_node(size_t rt, int k) {
		bool ret = false;
		Node node = get_node(rt);
		if (k != node.len - 1) {
			Node right = get_node(node.childs[k + 1]);
			if (right.len == (M + 1) / 2)
				ret = merge_nodes(rt, k);
			else
				adopt_from_right_node(rt, k);
		} else if (k) {
			Node left = get_node(node.childs[k - 1]);
			if (left.len == (M + 1) / 2)
				ret = merge_nodes(rt, k - 1);
			else
				adopt_from_left_node(rt, k);
		}
		return ret;
	}
	OperationResult erase(const Key & key, size_t rt, bool & small) {
		small = false;
		Node node = get_node(rt);
		int k;
		for (k = 0; k < node.len - 1 && !cmp(key, node.keys[k]); ++k);
		if (node.leaf) {
			Block block = get_block(node.childs[k]);
			int i;
			for (i = 0; i < block.len && cmp(block.keys[i], key); ++i);
			if (i == block.len || cmp(key, block.keys[i]))
				return Fail;
			for (int j = i; j < block.len - 1; ++j) {
				block.keys[j] = block.keys[j + 1];
				block.values[j] = block.values[j + 1];
			}
			--block.len;
			put_block(block, node.childs[k]);
			if (block.len < (L + 1) / 2)
				small = fix_small_block(rt, k);
		} else {
			bool flag;
			if (erase(key, node.childs[k], flag) == Fail)
				return Fail;
			if (flag)
				small = fix_small_node(rt, k);
		}
		return Success;
	}
public:
	// Default Constructor and Copy Constructor
	BTree() {
		std::ifstream info("info.dat", std::ios::binary);
		if (info) {
			info.read((char *) &root, sizeof(root));
			info.read((char *) &len, sizeof(len));
			info.close();
		} else
			initialize();
		nodes.open("nodes.dat", std::ios::in | std::ios::out | std::ios::binary);
		blocks.open("blocks.dat", std::ios::in | std::ios::out | std::ios::binary);
	}
	BTree(const BTree& other) {
		// Todo Copy
	}
	BTree& operator=(const BTree& other) {
		// Todo Assignment
	}
	~BTree() {
		std::ofstream info("info.dat", std::ios::binary);
		info.write((char *) &root, sizeof(root));
		info.write((char *) &len, sizeof(len));
		info.close();
		nodes.close();
		blocks.close();
	}
	// Insert: Insert certain Key-Value into the database
	// Return a pair, the first of the pair is the iterator point to the new
	// element, the second of the pair is Success if it is successfully inserted
	pair<iterator, OperationResult> insert(const Key& key, const Value& value) {
		bool large;
		pair<iterator, OperationResult> ret = insert(key, value, root, large);
		if (large) {
			Node rt = get_node(root);
			Node nrt(1, false);
			nrt.childs[0] = root;
			root = ntop();
			put_node(nrt, ntop());
			split_node(root, 0);
		}
		++len;
		return ret;
	}
	// Erase: Erase the Key-Value
	// Return Success if it is successfully erased
	// Return Fail if the key doesn't exist in the database
	OperationResult erase(const Key& key) {
		OperationResult ret;
		bool dummy;
		ret = erase(key, root, dummy);
		if (ret == Success) {
			Node node = get_node(root);
			if (node.len == 1)
				root = node.childs[0];
			--len;
		}
		return ret;  // If you can't finish erase part, just remaining here.
	}
	// Return a iterator to the beginning
	iterator begin() {}
	const_iterator cbegin() const {}
	// Return a iterator to the end(the next element after the last)
	iterator end() {}
	const_iterator cend() const {}
	// Check whether this BTree is empty
	bool empty() const {
		return len == 0;
	}
	// Return the number of <K,V> pairs
	size_t size() const {
		return len;
	}
	// Clear the BTree
	void clear() {
		nodes.close();
		blocks.close();
		initialize();
		nodes.open("nodes.dat", std::ios::in | std::ios::out | std::ios::binary);
		blocks.open("blocks.dat", std::ios::in | std::ios::out | std::ios::binary);
	}
	// Return the value refer to the Key(key)
	Value at(const Key& key){
		return at(key, root);
	}
	/**
	 * Returns the number of elements with key
	 *   that compares equivalent to the specified argument,
	 * The default method of check the equivalence is !(a < b || b > a)
	 */
	size_t count(const Key& key) const {}
	/**
	 * Finds an element with key equivalent to key.
	 * key value of the element to search for.
	 * Iterator to an element with key equivalent to key.
	 *   If no such element is found, past-the-end (see end()) iterator is
	 * returned.
	 */
	iterator find(const Key& key) {}
	const_iterator find(const Key& key) const {}
};
}  // namespace sjtu
