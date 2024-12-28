#ifndef __NODE_H
#define __NODE_H

#include <typeinfo>
#include <vector>

namespace Script
{

template <typename Type = void>
	class Value;

template <>
	class Value<void>
{
public:
	virtual ~Value() { }
protected:
	virtual const void * Get(const std::type_info& type) const
	{
		return nullptr;
	}
public:
	template <typename T>
		T * Get()
	{
		return reinterpret_cast<T *>(const_cast<void *>(Get(typeid(T))));
	}

	template <typename T>
		const T * Get() const
	{
		return reinterpret_cast<T *>(Get(typeid(T)));
	}
};

template <typename Type>
	class Value : public Value<>
{
public:
	Type value;

	Value(const Type& value)
		: value(value)
	{
	}

protected:
	const void * Get(const std::type_info& type) const override
	{
		if(typeid(Type) == type)
			return &value;
		else
			return nullptr;
	}
};

class Node;

class List
{
public:
	std::vector<Node *> children;

protected:
	void init() { }
	template <typename ... Nodes>
		void init(Node * node, Nodes ... nodes)
	{
		children.push_back(node);
		init(nodes...);
	}

public:
	template <typename ... Nodes>
		List(Nodes ... nodes)
	{
		init(nodes...);
	}

	List * Append(Node * node)
	{
		children.push_back(node);
		return this;
	}
};

class Node
{
public:
	enum node_type
	{
		Sequence, /* $... */
		CurrentAddress, /* here */
		Identifier, /* $s */
		Parameter, /* ?$s? */
		Integer, /* $i */
		BaseOf, /* base of $s */
		StartOf, /* start of $s */
		SizeOf, /* size of $s */
		Location, /* $1:$2 */
		Neg, /* -$1 */
		Not, /* ~$1 */
		AlignTo, /* align($1, $2) */
		Minimum, /* minimum($...) */
		Maximum, /* maximum($...) */
		Shl, /* $1<<$2 */
		Shr, /* $1>>$2 */
		Add, /* $1+$2 */
		Sub, /* $1-$2 */
		And, /* $1&$2 */
		Xor, /* $1^$2 */
		Or, /* $1|$2 */

		SetCurrentAddress, /* at $1 */
		AlignAddress, /* align $1 */
		SetNextBase, /* base $1 */
		Assign, /* $s = $1 */

		MatchAny, /* any */
		MatchName, /* $s */
		MatchSuffix, /* suffix $s */

		IsReadable, /* read */
		IsWritable, /* write */
		IsExecutable, /* execute */
		IsMergeable, /* merge */
		IsZeroFilled, /* zero */
		IsFixedAddress, /* fixed */
		IsResource, /* resource */
		IsOptional, /* optional */
		IsStack, /* stack */
		IsHeap, /* heap */
		IsCustomFlag, /* custom flags */
		Collect, /* all $1 $2 */
		NotPredicate, /* not $1 */
		AndPredicate, /* $1 and $2 */
		OrPredicate, /* $1 or $2 */
		MaximumSections, /* $1 maximum $2 */

		Segment, /* $s { $1 } $2 */
		SegmentTemplate, /* for $1 { $2 } $3 */
	} type;
	Value<> * value;
	List * list;

	Node(node_type type, Value<> * value, List * list)
		: type(type), value(value), list(list)
	{
	}

	Node(node_type type, Value<> * value)
		: type(type), value(value), list(new List)
	{
	}

	Node(node_type type, List * list)
		: type(type), value(new Value<>), list(list)
	{
	}

	Node(node_type type)
		: type(type), value(new Value<>), list(new List)
	{
	}

	Node *& at(size_t index)
	{
		return list->children[index];
	}

	Node * const& at(size_t index) const
	{
		return list->children[index];
	}
};

List * parse_string(const char * buffer);
List * parse_file(const char * filename);

}

extern void set_buffer(const char * buffer);
extern void set_stream(FILE * file);
extern int yylex(void);

#endif /* __NODE_H */
