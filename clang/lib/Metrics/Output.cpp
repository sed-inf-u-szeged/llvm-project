#include <clang/Metrics/Output.h>

#include <clang/AST/Mangle.h>
#include <clang/AST/AST.h>


using namespace clang::metrics;

namespace
{
  // Helper function to insert an element if the container does not contain it yet.
  // The inserted element will be default constructed.
  // If the element already exists, the function returns a pointer to it if condition is true.
  // Returns a pointer to the element, or nullptr, if no insertion took place and condition is false.
  template<class T> typename T::value_type::second_type* loadOnCondition(T& container, std::shared_ptr<UID> uid, bool condition)
  {
    auto it = container.find(uid);
    if (it == container.end())
      return &container[std::move(uid)];

    if (condition)
      return &it->second;

    return nullptr;
  }
}

// TODO: Remove this file!