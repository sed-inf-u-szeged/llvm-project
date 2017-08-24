#include "MetricsUtility.h"

#include <clang/AST/AST.h>


bool clang::metrics::isInterface(const clang::CXXRecordDecl* decl)
{
  // Ensure decl is of class or struct type (aka it's not a union).
  if (decl->isUnion())
    return false;

  // Ensure decl is not anonymous.
  if (decl->isAnonymousStructOrUnion())
    return false;

  // Iterate over the members.
  for (const Decl* member : decl->decls())
  {
    // If the member is a method.
    if (const CXXMethodDecl* md = dyn_cast<CXXMethodDecl>(member))
    {
      // Continue if m is pure virtual.
      if (md->isPure())
        continue;

      // Continue iteration if m is static.
      if (md->isStatic())
        continue;

      // Continue iteration if m was generated implicitly by the compiler.
      if (md->isImplicit())
        continue;

      // Continue iteration if m is a virtual destructor.
      if (CXXDestructorDecl::classofKind(md->getKind()) && md->isVirtual())
        continue;

      // Otherwise return false, as this method breaks the interface definition.
      return false;
    }
    else if (FieldDecl::classofKind(member->getKind()))
    {
      // By clang rules a "field" is non-static, so we can return false.
      return false;
    }
    else if (const VarDecl* vd = dyn_cast<VarDecl>(member))
    {
      // Static members must be const.
      if (!vd->getType().isConstQualified())
        return false;
    }
    else if (const CXXRecordDecl* rd = dyn_cast<CXXRecordDecl>(member))
    {
      // Any inner classes must be non-anonymous.
      if (rd->isAnonymousStructOrUnion())
        return false;
    }
  }

  // Recursively check each base class
  for (const CXXBaseSpecifier base : decl->bases())
  {
    if (!isInterface(base.getType()->getAsCXXRecordDecl()))
      return false;
  }

  // If everything went OK, the class is indeed an interface
  return true;
}