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
		virtual Value GetAttributes() const = 0;
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

	class ResourceManager
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

		virtual ~ResourceManager() = default;

		virtual std::shared_ptr<ResourceIterator> Iterate() = 0;
		std::shared_ptr<const ResourceIterator> Iterate() const
		{
			return std::const_pointer_cast<const ResourceIterator>(const_cast<ResourceManager *>(this)->Iterate());
		}
		virtual std::shared_ptr<ResourceIterator> EndOfIteration() = 0;
		std::shared_ptr<const ResourceIterator> EndOfIteration() const
		{
			return std::const_pointer_cast<const ResourceIterator>(const_cast<ResourceManager *>(this)->EndOfIteration());
		}

		ConstIterator begin() const
		{
			return ConstIterator{Iterate()};
		}

		ConstIterator end() const
		{
			return ConstIterator{EndOfIteration()};
		}

		Iterator begin()
		{
			return Iterator{Iterate()};
		}

		Iterator end()
		{
			return Iterator{EndOfIteration()};
		}

		virtual std::shared_ptr<ResourceIterator> FindResource(const Value& value) = 0;
		std::shared_ptr<const ResourceIterator> FindResource(const Value& value) const
		{
			return std::const_pointer_cast<const ResourceIterator>(const_cast<ResourceManager *>(this)->FindResource(value));
		}

		virtual void AddResource(std::shared_ptr<Resource> resource) = 0;
	};
}

#endif /* RESOURCE_H */
