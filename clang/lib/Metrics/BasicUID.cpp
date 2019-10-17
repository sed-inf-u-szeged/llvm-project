#include <clang/Metrics/BasicUID.h>

#include <clang/AST/ASTContext.h>
#include <llvm/Support/raw_ostream.h>
#include <clang/Metrics/RecursiveASTPrePostVisitor.h>


using namespace clang;

metrics::BasicUIDFactory::BasicUIDFactory()
  : pMyASTCtx (nullptr)
  , diagnosticsEngine(IntrusiveRefCntPtr<clang::DiagnosticIDs>(new DiagnosticIDs()),
                      &*IntrusiveRefCntPtr<clang::DiagnosticOptions>(new DiagnosticOptions()))
{
}

void metrics::BasicUIDFactory::onSourceOperationBegin(clang::ASTContext& context, llvm::StringRef filename)
{
  pMyASTCtx = &context;
  pMyMangleCtx.reset(clang::ItaniumMangleContext::create(context, diagnosticsEngine));
}

std::unique_ptr<metrics::UID> metrics::BasicUIDFactory::create(const clang::Decl* decl)
{
  std::string mangledName;
  llvm::raw_string_ostream ss(mangledName);

  if (decl)
  {
    const SourceManager& sm = pMyASTCtx->getSourceManager();
    const DeclContext *parent = nullptr;

    // Functions can have their name mangled easily by the built-in mangler
    if (FunctionDecl::classof(decl))
    {
      // If this function is only visible from the current translation unit (eg.: static functions, functions in anonymous namespace)
      if (cast<FunctionDecl>(decl)->getLinkageAndVisibility().getLinkage() != Linkage::ExternalLinkage)
      {
        // Put the filepath into the stream, prefixed by a "//" and break.
        ss << "//" << sm.getFilename(cast<FunctionDecl>(decl)->getLocation());
      }

      // Use the built-in mangler to get the mangled name for the function.
      if (CXXConstructorDecl::classof(decl))
        pMyMangleCtx->mangleCXXCtor(cast<CXXConstructorDecl>(decl), CXXCtorType::Ctor_Complete, ss);
      else if (CXXDestructorDecl::classof(decl))
        pMyMangleCtx->mangleCXXDtor(cast<CXXDestructorDecl>(decl), CXXDtorType::Dtor_Complete, ss);
      else
        pMyMangleCtx->mangleName(cast<FunctionDecl>(decl), ss);
    }
    else
    {
      if (const FriendDecl* fd = dyn_cast<FriendDecl>(decl))
        parent = fd->getDeclContext();
      else
        parent = clang::dyn_cast_or_null<DeclContext>(decl);
    }
    if (parent)
    {
      // For non-functions the qualified name identifies the object - only need to take care of cases where decl
      // is within an anonymous namespace or class.
      // Iterate through the AST hierarchy to check if there's an anonymous parent.
      while (parent)
      {
        if (NamespaceDecl::classofKind(parent->getDeclKind()))
        {
          const NamespaceDecl* ns = cast<NamespaceDecl>(parent);
          if (ns->isAnonymousNamespace())
          {
            // Put the filepath into the stream, prefixed by a "//" and break.
            ss << "//" << sm.getFilename(ns->getLocation());
            break;
          }
        }
        else if (CXXRecordDecl::classofKind(parent->getDeclKind()))
        {
          const CXXRecordDecl* cs = cast<CXXRecordDecl>(parent);
          if (cs->isAnonymousStructOrUnion())
          {
            // Put the filepath into the stream, prefixed by a "//".
            ss << "//" << sm.getFilename(cs->getLocation());

            // Also put the line info into the stream, as there can be multiple anonymous classes within the same file.
            ss << "//" << sm.getExpansionLineNumber(cs->getLocStart()) << '_' << sm.getExpansionLineNumber(cs->getLocEnd());
            ss << '_' << sm.getExpansionColumnNumber(cs->getLocStart()) << '_' << sm.getExpansionColumnNumber(cs->getLocEnd());

            break;
          }
        }

        // Continue the loop with the parent of the parent
        parent = parent->getParent();
      }

      if (const FriendDecl *fd = dyn_cast<FriendDecl>(decl))
      {
        if (const Type* type = fd->getFriendType()->getType().getTypePtrOrNull())
          if (const CXXRecordDecl* classDecl = type->getAsCXXRecordDecl())
          {
            decl = classDecl;
          }
      }

      // Append the qualified name to the stream
      const NamedDecl *nd = dyn_cast<NamedDecl>(decl);
      if (nd)
        nd->printQualifiedName(ss);
      else
        ss << "<missing qualified name>";
    }
    else
    {
      ss << "<missing id>";
    }
  }
  else
  {
    ss << "<missing id>";
  }

  return std::unique_ptr<BasicUID>(new BasicUID(ss.str()));
}
