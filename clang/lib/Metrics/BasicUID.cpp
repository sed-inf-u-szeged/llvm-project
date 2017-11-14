#include <clang/Metrics/BasicUID.h>

#include <clang/AST/ASTContext.h>
#include <llvm/Support/raw_ostream.h>


using namespace clang;

metrics::BasicUIDFactory::BasicUIDFactory()
  : pMyASTCtx (nullptr)
  , diagnosticsEngine(IntrusiveRefCntPtr<clang::DiagnosticIDs>(new DiagnosticIDs()),
                      &*IntrusiveRefCntPtr<clang::DiagnosticOptions>(new DiagnosticOptions()))
{
}

void metrics::BasicUIDFactory::onSourceOperationEnd(clang::ASTContext& context)
{
  pMyASTCtx = &context;
  pMyMangleCtx.reset(clang::ItaniumMangleContext::create(context, diagnosticsEngine));
}

std::unique_ptr<metrics::UID> metrics::BasicUIDFactory::create(const clang::Decl* decl)
{
  std::string mangledName;
  llvm::raw_string_ostream ss(mangledName);

  const SourceManager& sm = pMyASTCtx->getSourceManager();

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
  else if (const DeclContext* parent = clang::dyn_cast_or_null<DeclContext>(decl))
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
          ss << "//" << sm.getSpellingLineNumber(cs->getLocStart()) << '_' << sm.getSpellingLineNumber(cs->getLocEnd());
          ss << '_' << sm.getSpellingColumnNumber(cs->getLocStart()) << '_' << sm.getSpellingColumnNumber(cs->getLocEnd());

          break;
        }
      }

      // Continue the loop with the parent of the parent
      parent = parent->getParent();
    }

    // Append the qualified name to the stream
    const NamedDecl* nd = cast<NamedDecl>(decl);
    nd->printQualifiedName(ss);
  }
  else
  {
    ss << "<missing id>";
  }

  return std::unique_ptr<BasicUID>(new BasicUID(ss.str()));
}
