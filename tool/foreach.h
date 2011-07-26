// Public domain.

#ifndef __FOREACH_H__
#define __FOREACH_H__

namespace foreach_ns
{
	struct ContainerBase
	{
	};

	template <typename Ty> class Container : public ContainerBase
	{
	public:
		inline Container(const Ty& _container)
			: m_container(_container)
			, m_break(0)
			, m_it( _container.begin() )
			, m_itEnd( _container.end() )
		{
		}

		inline bool condition() const
		{
			return (!m_break++ && m_it != m_itEnd);
		}

		const Ty& m_container;
		mutable int m_break;
		mutable typename Ty::const_iterator m_it;
		mutable typename Ty::const_iterator m_itEnd;
	};

	template <typename Ty>
	inline Ty* pointer(const Ty&)
	{
		return 0;
	}

	template <typename Ty>
	inline Container<Ty> containerNew(const Ty& _container)
	{
		return Container<Ty>(_container);
	}

	template <typename Ty>
	inline const Container<Ty>* container(const ContainerBase* _base, const Ty*)
	{
		return static_cast<const Container<Ty>*>(_base);
	}

} // namespace foreach_ns

#define foreach(_variable, _container) \
			for (const foreach_ns::ContainerBase &__temp_container__ = foreach_ns::containerNew(_container); \
				foreach_ns::container(&__temp_container__, true ? 0 : foreach_ns::pointer(_container))->condition(); \
				++foreach_ns::container(&__temp_container__, true ? 0 : foreach_ns::pointer(_container))->m_it) \
				for (_variable = *container(&__temp_container__, true ? 0 : foreach_ns::pointer(_container))->m_it; \
					foreach_ns::container(&__temp_container__, true ? 0 : foreach_ns::pointer(_container))->m_break; \
					--foreach_ns::container(&__temp_container__, true ? 0 : foreach_ns::pointer(_container))->m_break)

#endif // __FOREACH_H__

