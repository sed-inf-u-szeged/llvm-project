#pragma once

#include <cstddef>


namespace clang
{
  namespace metrics
  {
    /*!
     *  Interface allowing the user to define unique identifiers for AST nodes.
     *
     *  \sa UIDFactory, Output
     */
    class UID
    {
    public:
      virtual ~UID() {}

      //! Returns true if and only if two UIDs are considered equal.
      //!  \param rhs UID to compare to
      virtual bool equals(const UID& rhs) const = 0;

      //! Returns a hash value for this UID.
      //! Two UIDs for which UID::equals() is true must also return the same hash.
      //! The hash of a UID must remain constant through its lifetime.
      virtual std::size_t hash() const = 0;
    };
  }
}