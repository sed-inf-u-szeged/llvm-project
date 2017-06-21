#include "CMA_common.h"

void ClangMetricsAction::addMatcherFunctionDefinitions()
{
	// Matching functions which also have a definition but are not template instantiations
	DeclarationMatcher functions = functionDecl(isDefinition(), unless(isTemplateInstantiation()), unless(isImplicit())).bind("functionsWithBody");
	myFinder.addMatcher(functions, [this](const MatchFinder::MatchResult& result)
	{
		if (const FunctionDecl* match = result.Nodes.getNodeAs<FunctionDecl>("functionsWithBody"))
		{
			// Calculate LOC
			auto loc = calculateLOC(match->getSourceRange());

			// We store the calculated metrics. Later, we'll substract local classes from LOC and LLOC.
			storeLOC(loc, match, myFunctionSizeMetrics);

			// We increase the McCC for each function by one. This is because of the McCC definition, so that even functions
			// without any branches have a minimum complexity of one.
			++myFunctionMcCC[match];

			// Finally, if this is a method, we also add its LOC to the LOC of its class.
			if (CXXMethodDecl::classofKind(match->getKind()))
			{
				// Get the class to which this method belongs
				const CXXRecordDecl* parentClass = static_cast<const CXXMethodDecl*>(match)->getParent();

				// We only need to handle it seperately if the method is NOT inline defined within the class
				if (!containsRange(match->getSourceRange(), parentClass->getSourceRange()))
				{
					myClassSizeMetrics.TLOC[parentClass]  += loc.first + loc.second;
					myClassSizeMetrics.LOC[parentClass]   += loc.first + loc.second;
					myClassSizeMetrics.TLLOC[parentClass] += loc.first;
					myClassSizeMetrics.LLOC[parentClass]  += loc.first;
				}
			}
		}
	});

	// Matchig classes which are defined inside functions
	DeclarationMatcher insideClasses = recordDecl(anyOf(isClass(), isStruct(), isUnion()), isDefinition(), hasAncestor(functions)).bind("functionLocalClasses");
	myFinder.addMatcher(insideClasses, [this](const MatchFinder::MatchResult& result)
	{
		if (const RecordDecl* match = result.Nodes.getNodeAs<RecordDecl>("functionLocalClasses"))
		{
			// Find parent function in which this class definition is made
			const FunctionDecl* parentFunction = static_cast<const FunctionDecl*>(match->getParentFunctionOrMethod());

			// Subtract the size of this class definition from the function's LOC/LLOC
			auto loc = calculateLOC(match->getSourceRange());
			myFunctionSizeMetrics.LOC[parentFunction]  -= loc.first + loc.second;
			myFunctionSizeMetrics.LLOC[parentFunction] -= loc.first;
		}
	});
}