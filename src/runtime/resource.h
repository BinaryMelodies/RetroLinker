#ifndef RESOURCE_H
#define RESOURCE_H

#include <memory>
#include "value.h"

namespace Runtime
{
	class Resource
	{
	public:
		virtual ~Resource() = default;
		virtual Value GetProperties() const = 0;
	};

	class ResourceIterator
	{
	public:
		virtual ~ResourceIterator() = default;
		virtual void Step() const = 0;
		virtual std::shared_ptr<const Resource> Get() const = 0;
		std::shared_ptr<Resource> Get()
		{
			return std::const_pointer_cast<Resource>(const_cast<const ResourceIterator *>(this)->Get());
		}
		virtual bool SameAs(const ResourceIterator * other) const = 0;
		virtual void Erase() = 0;
	};

	class ResourceIterable
	{
	public:
		struct Iterator
		{
			std::shared_ptr<ResourceIterator> iterator_object;

			std::shared_ptr<Resource> operator *()
			{
				return iterator_object->Get();
			}

			Iterator& operator ++()
			{
				iterator_object->Step();
				return *this;
			}

			bool operator !=(const Iterator& other) const
			{
				return !iterator_object->SameAs(other.iterator_object.get());
			}
		};

		struct ConstIterator
		{
			std::shared_ptr<const ResourceIterator> iterator_object;

			std::shared_ptr<const Resource> operator *()
			{
				return iterator_object->Get();
			}

			ConstIterator& operator ++()
			{
				iterator_object->Step();
				return *this;
			}

			bool operator !=(const ConstIterator& other) const
			{
				return !iterator_object->SameAs(other.iterator_object.get());
			}
		};

		virtual std::shared_ptr<ResourceIterator> BeginIteration() = 0;
		std::shared_ptr<const ResourceIterator> BeginIteration() const
		{
			return std::const_pointer_cast<const ResourceIterator>(const_cast<ResourceIterable *>(this)->BeginIteration());
		}
		virtual std::shared_ptr<ResourceIterator> EndOfIteration() = 0;
		std::shared_ptr<const ResourceIterator> EndOfIteration() const
		{
			return std::const_pointer_cast<const ResourceIterator>(const_cast<ResourceIterable *>(this)->EndOfIteration());
		}

		ConstIterator begin() const
		{
			return ConstIterator{BeginIteration()};
		}

		ConstIterator end() const
		{
			return ConstIterator{EndOfIteration()};
		}

		Iterator begin()
		{
			return Iterator{BeginIteration()};
		}

		Iterator end()
		{
			return Iterator{EndOfIteration()};
		}
	};

	class ResourceManager
	{
	public:
		virtual ~ResourceManager() = default;

		virtual ResourceIterable& Iterate() = 0;
		const ResourceIterable& Iterate() const
		{
			return const_cast<const ResourceIterable&>(const_cast<ResourceManager *>(this)->Iterate());
		}

		virtual std::shared_ptr<ResourceIterator> FindResource(const Value& value) = 0;
		std::shared_ptr<const ResourceIterator> FindResource(const Value& value) const
		{
			return std::const_pointer_cast<const ResourceIterator>(const_cast<ResourceManager *>(this)->FindResource(value));
		}

		virtual bool UpdateResource(std::shared_ptr<Resource> resource) = 0;
	};
}

#endif /* RESOURCE_H */
