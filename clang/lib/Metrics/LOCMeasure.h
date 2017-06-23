#pragma once

#include <clang/Frontend/CompilerInstance.h>

#include <vector>
#include <algorithm>


namespace clang
{
namespace metrics
{
	namespace detail
	{
		//! Helper class for LOC/LLOC calculation.
		class LOCMeasure final
		{
		private:
			// Forward declaration.
			template<class Map> class MergeOption_helper;

			// Stores info for calculate().
			struct LocInfo
			{
				bool isIgnored;
				unsigned startingLine;
				unsigned endingLine;
			};

		public:
			//! Stores lines of code (total and logical) returned by calculate().
			struct LOC
			{
				//! Lines of code including empty and comment lines.
				unsigned total;

				//! Lines of code excluding empty and comment lines.
				unsigned logical;
			};

		public:
			//! Constructor.
			//!  \param sm reference to the current clang::SourceManager
			//!  \param codeLines set of line numbers where logical code is written (needed for LLOC)
			LOCMeasure(const clang::SourceManager& sm, const std::set<unsigned>& codeLines) : rMySm(sm), rMyCodeLines(codeLines)
			{}

			//! Calculates the LOC/LLOC of an AST object of type T (eg.: clang::CXXRecordDecl).
			//!
			//! T must be a type for which getLocStart() and getLocEnd() functions are callable through the arrow operator ("->").
			//! These functions must both return a clang::SourceLocation.
			//!
			//! Takes any number of optional subMaps (wrapped in an ignore/merge option), which define source ranges inside object's
			//! that should not be counted (ignore option) or should be merged additionally to the object's LOC (merge option).
			//! The type of each subMap must be [unordered_]map<[const] T*, [unordered_]set<[const] S*>>, where S is an arbitrary
			//! type for which the getLocStart() -> clang::SourceLocation and getLocEnd() -> clang::SourceLocation methods are defined.
			//!
			//! Example:
			//! When calculating the LOC/LLOC of a namespace, inner namespaces need to be ignored. Thus the function would
			//! be called as the following: calculate( namespaceInQuestion, ignore(mapOfInnerNamespacesByNamespaces) ), where
			//!    namespaceInQuestion is a const NamespaceDecl*,
			//!    mapOfInnerNamespacesByNamespaces is an unordered_map<const NamespaceDecl*, unordered_set<const NamespaceDecl*>>.
			//! To calculate TLOC, the function would be called without providing any options.
			//!
			//! \param object the AST node in question
			//! \param options an optional list of ignore() and merge() options
			//! \return The total and logical lines of code wrapped in a LOC object.
			template<class T, class... MergeOptions>
			LOC calculate(const T* object, const MergeOptions&... options);

			//! Calculate option "merge".
			//!  \param subMap the map on which the option takes effect
			//!  \{param} rangeOnly defines whether to merge only within the range of the main object when using calculate()
			//!           Example: Methods outside of a class definition's source range need to be taken into account too,
			//!                    so this option would be false for class LOC.
			//!  \return Proxy object for merge option.
			//!  \sa calculate
			template<class Map> static MergeOption_helper<Map> merge(const Map& subMap, bool rangeOnly = true)
			{
				return MergeOption_helper<Map>(subMap, false, rangeOnly);
			}

			//! Calculate option "ignore".
			//!  \param subMap the map on which the option takes effect
			//!  \param rangeOnly defines whether to ignore only within the range of the main object when using calculate()
			//!  \return Proxy object for ignore option.
			//!  \sa calculate
			template<class Map> static MergeOption_helper<Map> ignore(const Map& subMap, bool rangeOnly = true)
			{
				return MergeOption_helper<Map>(subMap, true, rangeOnly);
			}

		private:
			// Reference to the SourceManager from the FrontendAction.
			const clang::SourceManager& rMySm;

			// Contains the line numbers where logical code (aka neither comment, nor empty line) is written.
			const std::set<unsigned>& rMyCodeLines;


		private:
			// Helper functions for calculate. Should not be called manually.
			template<class T, class MergeOption, class... MergeOptions>
			void mergeLOC_helper(const T* object, std::vector<LocInfo>& order, const MergeOption& option, const MergeOptions&... options);
			template<class T>
			void mergeLOC_helper(const T* object, std::vector<LocInfo>& order) {}

			// Helper class for calculate().
			template<class Map>
			class MergeOption_helper
			{
			private:
				MergeOption_helper(const Map& subMap, bool isIgnore, bool rangeOnly) : mySubMap(subMap), myIsIgnore(isIgnore), myIsRangeOnly(rangeOnly)
				{}

				friend MergeOption_helper LOCMeasure::merge<Map>(const Map&, bool);
				friend MergeOption_helper LOCMeasure::ignore<Map>(const Map&, bool);

			public:
				const Map& getMap() const { return mySubMap; }

				// True exactly if this is an ignore option, false if it is a merge option.
				bool isIgnore() const { return myIsIgnore; }

				// True exactly if merging/ignoring should be done within the object's source range.
				bool isRangeOnly() const { return myIsRangeOnly; }

			private:
				const Map& mySubMap;
				bool myIsIgnore;
				bool myIsRangeOnly;
			};
		};



		// --------------------------- METHOD DEFINITIONS ------------------------------------------------------------------------

		template<class T, class... MergeOptions>
		LOCMeasure::LOC LOCMeasure::calculate(const T* object, const MergeOptions&... options)
		{
			using namespace clang;

			// Helper lambda
			// Returns the number of logical lines between from and to
			auto calculateLLOC = [this](unsigned from, unsigned to)
			{
				auto start = rMyCodeLines.lower_bound(from);
				auto end = rMyCodeLines.lower_bound(to);
				return std::distance(start, end) + 1;
			};

			// We store the locations to ignor in this vector, which we are going to order later.
			std::vector<LocInfo> order;

			// LocInfo of the current object
			LocInfo objInfo;
			objInfo.startingLine = rMySm.getSpellingLineNumber(object->getLocStart());
			objInfo.endingLine   = rMySm.getSpellingLineNumber(object->getLocEnd());

			// Calculate the LOC/LLOC of the object in question (we are going to subtract the ignored ranges from this later)
			unsigned loc  = objInfo.endingLine - objInfo.startingLine + 1;
			unsigned lloc = calculateLLOC(objInfo.startingLine, objInfo.endingLine);

			// Use the helper to put every element needed to be ignored into the vector
			mergeLOC_helper(object, order, options...);

			// If order is empty, it means that there aren't any nodes to ignore/merge - we can simply return the LOC already
			if (order.empty())
				return { loc, lloc };

			// Remove all "duplicates". A duplicates are pairs which both start and end on the same line regardless of ignore
			// setting. This is an important step that ensures the correct LOC calculation.
			// To do this, we convert the vector to a set.
			auto smaller = [](const LocInfo& lhs, const LocInfo& rhs)
			{
				if (lhs.startingLine == rhs.startingLine)
					return lhs.endingLine < rhs.endingLine;

				return lhs.startingLine < rhs.startingLine;
			};
			std::set<LocInfo, decltype(smaller)> orderedSet(smaller);

			// Manually insert all elements
			for (const LocInfo& info : order)
			{
				auto result = orderedSet.insert(info);
		
				// If there was already an element like this, we remove it
				if (!result.second)
					orderedSet.erase(result.first);
			}

			// If order is empty, it means that there aren't any nodes to ignore/merge - we can simply return the LOC already
			if (orderedSet.empty())
				return { loc, lloc };

			// TODO: This would be unnecessary! Get rid of the vector and use the set everywhere!
			// Convert back to vector
			order = std::vector<LocInfo>(orderedSet.begin(), orderedSet.end());

			// Correct LOC for the first element (first element always exists at this point)
			if (order[0].isIgnored)
			{
				// Ignore this range
				loc  -= order[0].endingLine - order[0].startingLine + 1;
				lloc -= calculateLLOC(order[0].startingLine, order[0].endingLine);

				// If the first element starts on the same line as the node in question, we must correct the LOC by adding one
				if (objInfo.startingLine == order[0].startingLine) ++loc, ++lloc;

				// If the last element (which can be the same as the first) ends on the same line as the object, we also need to add one
				if (objInfo.endingLine == order.back().endingLine) ++loc, ++lloc;
			}
			else
			{
				// Merge this range
				loc  += order[0].endingLine - order[0].startingLine + 1;
				lloc += calculateLLOC(order[0].startingLine, order[0].endingLine);

				// If the first element starts on the same line as the node in question, we must correct the LOC by subtracting one
				if (objInfo.startingLine == order[0].startingLine) --loc, --lloc;

				// If the last element (which can be the same as the first) ends on the same line as the object, we also need to subtract one
				if (objInfo.endingLine == order.back().endingLine) --loc, --lloc;
			}

			// Iterate over the vector
			for (size_t i = 1; i < order.size(); ++i)
			{
				// Look for entries that start on the same line where the previous one ends
				if (order[i].startingLine == order[i - 1].endingLine)
				{
					// Correct the LOC (otherwise this line would be counted twice)
					if (order[i].isIgnored)
					{
						++loc;
						++lloc;
					}
					else
					{
						--loc;
						--lloc;
					}
				}

				// Ignore or merge to LOC
				const unsigned correctionLOC  = order[i].endingLine - order[i].startingLine + 1;
				const unsigned correctionLLOC = calculateLLOC(order[i].startingLine, order[i].endingLine);
				if (order[i].isIgnored)
				{
					loc  -= correctionLOC;
					lloc -= correctionLLOC;
				}
				else
				{
					loc  += correctionLOC;
					lloc += correctionLLOC;
				}

			}

			return { loc, lloc };
		}

		template<class T, class MergeOption, class... MergeOptions>
		void LOCMeasure::mergeLOC_helper(const T* object, std::vector<LocInfo>& order, const MergeOption& option, const MergeOptions&... options)
		{
			// Lookup the object in the map
			auto it = option.getMap().find(object);

			// If it doesn't exist within the map, we continue with the other maps
			if (it == option.getMap().end())
				return mergeLOC_helper(object, order, options...);

			// Otherwise we can now reserve the required space in the vector
			order.reserve(order.size() + it->second.size());

			// Source range of the object
			unsigned oStart = rMySm.getSpellingLineNumber(object->getLocStart());
			unsigned oEnd   = rMySm.getSpellingLineNumber(object->getLocEnd());

			// Insert the location of every node into the vector
			for (auto d : it->second)
			{
				LocInfo info;
				info.startingLine = rMySm.getSpellingLineNumber(d->getLocStart());
				info.endingLine   = rMySm.getSpellingLineNumber(d->getLocEnd());
				if (option.isRangeOnly())
				{
					if (info.startingLine < oStart || info.endingLine > oEnd)
						continue;
				}
				info.isIgnored = option.isIgnore();
				order.push_back(info);
			}

			// Continue filling the vector from the other maps
			mergeLOC_helper(object, order, options...);
		}
}
}
}
