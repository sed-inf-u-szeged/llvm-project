#include <clang/Metrics/MetricsUtility.h>
#include<vector>
#include<iostream>

#include <clang/AST/AST.h>


bool clang::metrics::isInterface(const clang::CXXRecordDecl* decl)
{
  if (decl == nullptr)
    return false;

  // If this is a declaration and we can't find no definition, then we assume it's not an interface... we can't do much (unless we link or something)
  if (!decl->hasDefinition())
    return false;

  decl = decl->getDefinition();

  // Ensure decl is of class or struct type (aka it's not a union).
  if (decl->isUnion())
    return false;

  // Ensure decl is not anonymous.
  if (decl->isAnonymousStructOrUnion())
    return false;

  // Iterate over the members.
  for (const Decl* member : decl->decls())
  {
    if (const FunctionTemplateDecl *ftd = dyn_cast<FunctionTemplateDecl>(member))
    {
      // Handle the underlying method.
      member = ftd->getTemplatedDecl();
    }
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

  // If everything went OK, check for the existence of the virtual destructor.
  //const CXXDestructorDecl* dd = decl->getDestructor();
  //return dd && dd->isVirtual();
  return true;
}

bool clang::manuallyExpandClassScopeFunctionSpecializationDecl(const clang::ClassScopeFunctionSpecializationDecl* decl, clang::ASTContext &context)
{
  for (auto res : decl->getDeclContext()->lookup(decl->getSpecialization()->getNameInfo().getName()))
  {
    if (FunctionTemplateDecl *templDecl = dyn_cast_or_null<FunctionTemplateDecl>(res))
    {
      std::vector<TemplateArgument> myTArgList;
      unsigned parameterIndex = 0;
      for (auto param : templDecl->getAsFunction()->parameters())
      {
        if(param->getType()->isTemplateTypeParmType()){
          myTArgList.push_back(TemplateArgument(decl->getSpecialization()->parameters()[parameterIndex]->getType()));
        }
        parameterIndex++;
      }

      //std::cout << "has: " << decl->hasExplicitTemplateArgs() << std::endl;

      decl->getSpecialization()->setFunctionTemplateSpecialization(templDecl,TemplateArgumentList::CreateCopy(context,ArrayRef<TemplateArgument>(myTArgList))
        ,nullptr,TSK_ExplicitSpecialization,&decl->templateArgs(),decl->getSpecialization()->getSourceRange().getBegin());
      context.setClassScopeSpecializationPattern(decl->getSpecialization(),templDecl->getAsFunction());

      return true;
    }
  }
  return false;
}

bool clang::isLambda(const FunctionDecl *decl) {
  const CXXRecordDecl *recordParent = dyn_cast_or_null<CXXRecordDecl>(decl->getParent());
  return recordParent && recordParent->isLambda();
}
