#pragma once

#include "Metrics.h"
#include "UIDFactory.h"

#include <memory>
#include <unordered_map>


namespace clang
{
	class FunctionDecl;
	class CXXRecordDecl;
	class EnumDecl;
	class NamespaceDecl;

	namespace metrics
	{
		namespace detail
		{
			class ClangMetricsAction;
		}

		/*!
		 *	The output is stored in an object of this class.
		 *	After each source operation, the calculated metrics are merged into the Output object.
		 *
		 *  To allow accessing data in an extendable way, a special UID system is used. An Output object
		 *  is constructed from a reference to a metrics::UIDFactory, which will be used internally to create
		 *  metrics::UIDs for cross-source identification of AST elements.
		 *
		 *  \sa UIDFactory, UID
		 */
		class Output final
		{
		private:
			// Helper for UID hashing.
			struct UIDHasher
			{
				typedef std::shared_ptr<const UID> argument_type;
				typedef size_t result_type;

				result_type operator()(const argument_type& o) const
				{
					return o->hash();
				}
			};

			// Helper for UID equivalence.
			struct UIDEq
			{
				typedef std::shared_ptr<const UID> first_argument_type;
				typedef std::shared_ptr<const UID> second_argument_type;
				typedef bool result_type;

				result_type operator()(const first_argument_type& lhs, const second_argument_type& rhs) const
				{
					return lhs->equals(*rhs);
				}
			};

			// Helper for iterator range-for wrappers.
			template<class T> class ItHelper
			{
			public:
				ItHelper(T b, T e) : myBegin(b), myEnd(e)
				{}

				T begin() { return myBegin; }
				T end() { return myEnd; }

			private:
				T myBegin, myEnd;
			};

		public:
			//! Constructor.
			//!  \{param} idFactory reference to the UIDFactory object (note: it is the responsibility of the caller
			//!           to handle the lifetime of idFactory so that it remains valid for the lifetime of Output)
			Output(UIDFactory& idFactory) : rMyFactory(idFactory)
			{}

			//! Access the function metrics for the specified UID.
			//!  \return pointer to the metrics or nullptr if there are no recorded metrics for this id
			const FunctionMetrics* getFunctionMetrics(const UID& id) const
			{
				return getMetrics(myFunctionMetrics, id);
			}

			//! Access the class metrics for the specified UID.
			//!  \return pointer to the metrics or nullptr if there are no recorded metrics for this id
			const ClassMetrics* getClassMetrics(const UID& id) const
			{
				return getMetrics(myClassMetrics, id);
			}

			//! Access the enum metrics for the specified UID.
			//!  \return pointer to the metrics or nullptr if there are no recorded metrics for this id
			const EnumMetrics* getEnumMetrics(const UID& id) const
			{
				return getMetrics(myEnumMetrics, id);
			}

			//! Access the namespace metrics for the specified UID.
			//!  \return pointer to the metrics or nullptr if there are no recorded metrics for this id
			const NamespaceMetrics* getNamespaceMetrics(const UID& id) const
			{
				return getMetrics(myNamespaceMetrics, id);
			}

			//! Access the file metrics for the specified filename.
			//!  \return pointer to the metrics or nullptr if there are no recorded metrics for this file
			const FileMetrics* getFileMetrics(const std::string& file) const
			{
				auto it = myFileMetrics.find(file);
				if (it != myFileMetrics.end())
					return &it->second;

				return nullptr;
			}

			//! Returns a reference to the internal UIDFactory received on construction.
			UIDFactory& getFactory() const
			{
				return rMyFactory;
			}

		private:
			// Adds new results to the stored ones. Called by ClangMetricsAction at the end of each source operation.
			friend class detail::ClangMetricsAction;
			void mergeFunctionMetrics(const clang::FunctionDecl* decl, const FunctionMetrics& m);
			void mergeClassMetrics(const clang::CXXRecordDecl* decl, const ClassMetrics& m, unsigned tlocT_raw, unsigned tlocL_raw, unsigned locT_raw, unsigned locL_raw);
			void mergeEnumMetrics(const clang::EnumDecl* decl, const EnumMetrics& m);
			void mergeNamespaceMetrics(const clang::NamespaceDecl* decl, const NamespaceMetrics& m);
			void mergeFileMetrics(const std::string& file, const FileMetrics& m);

		private:
			UIDFactory& rMyFactory;

			template<class T> auto getMetrics(const T& from, const UID& id) const -> decltype(&from.find(std::shared_ptr<const UID>())->second)
			{
				std::shared_ptr<const UID> dummy(std::shared_ptr<const UID>(), &id);
				auto it = from.find(dummy);
				if (it != from.end())
					return &it->second;

				return static_cast<decltype(&it->second)>(nullptr);
			}

			std::unordered_map<std::shared_ptr<const UID>, FunctionMetrics, UIDHasher, UIDEq>  myFunctionMetrics;
			std::unordered_map<std::shared_ptr<const UID>, ClassMetrics, UIDHasher, UIDEq>     myClassMetrics;
			std::unordered_map<std::shared_ptr<const UID>, EnumMetrics, UIDHasher, UIDEq>      myEnumMetrics;
			std::unordered_map<std::shared_ptr<const UID>, NamespaceMetrics, UIDHasher, UIDEq> myNamespaceMetrics;

			std::unordered_map<std::string, FileMetrics> myFileMetrics;

		public:
			// Iterator types
			typedef decltype(myFunctionMetrics)::const_iterator  function_iterator;
			typedef decltype(myClassMetrics)::const_iterator     class_iterator;
			typedef decltype(myEnumMetrics)::const_iterator      enum_iterator;
			typedef decltype(myNamespaceMetrics)::const_iterator namespace_iterator;
			typedef decltype(myFileMetrics)::const_iterator      file_iterator;

			//! Returns a const ForwardIterator to the beginning of the internal function metrics structure.
			//! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, FunctionMetrics>.
			//! The order of iteration is undefined, but all elements are guaranteed to be iterated over until function_end() is reached.
			function_iterator function_begin() { return myFunctionMetrics.begin(); }

			//! Returns a const ForwardIterator to the end of the internal function metrics structure.
			//! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, FunctionMetrics>.
			function_iterator function_end() { return myFunctionMetrics.end(); }

			//! Returns a const ForwardIterator to the beginning of the internal class metrics structure.
			//! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, ClassMetrics>.
			//! The order of iteration is undefined, but all elements are guaranteed to be iterated over until class_end() is reached.
			class_iterator class_begin() { return myClassMetrics.begin(); }

			//! Returns a const ForwardIterator to the end of the internal class metrics structure.
			//! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, ClassMetrics>.
			class_iterator class_end() { return myClassMetrics.end(); }

			//! Returns a const ForwardIterator to the beginning of the internal enum metrics structure.
			//! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, EnumMetrics>.
			//! The order of iteration is undefined, but all elements are guaranteed to be iterated over until enum_end() is reached.
			enum_iterator enum_begin() { return myEnumMetrics.begin(); }

			//! Returns a const ForwardIterator to the end of the internal enum metrics structure.
			//! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, EnumMetrics>.
			enum_iterator enum_end() { return myEnumMetrics.end(); }

			//! Returns a const ForwardIterator to the beginning of the internal namespace metrics structure.
			//! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, NamespaceMetrics>.
			//! The order of iteration is undefined, but all elements are guaranteed to be iterated over until namespace_end() is reached.
			namespace_iterator namespace_begin() { return myNamespaceMetrics.begin(); }

			//! Returns a const ForwardIterator to the end of the internal namespace metrics structure.
			//! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, NamespaceMetrics>.
			namespace_iterator namespace_end() { return myNamespaceMetrics.end(); }

			//! Returns a const ForwardIterator to the beginning of the internal file metrics structure.
			//! The iterator's value_tpye is std::pair<std::string, FileMetrics>.
			//! The order of iteration is undefined, but all elements are guaranteed to be iterated over until file_end() is reached.
			file_iterator file_begin() { return myFileMetrics.begin(); }

			//! Returns a const ForwardIterator to the end of the internal file metrics structure.
			//! The iterator's value_tpye is std::pair<std::string, FileMetrics>.
			file_iterator file_end() { return myFileMetrics.end(); }


			//! Iterator wrappers for use in range based for loops.
			ItHelper<function_iterator> functions() { return ItHelper<function_iterator>(function_begin(), function_end()); }
			ItHelper<class_iterator> classes() { return ItHelper<class_iterator>(class_begin(), class_end()); }
			ItHelper<enum_iterator> enums() { return ItHelper<enum_iterator>(enum_begin(), enum_end()); }
			ItHelper<namespace_iterator> namespaces() { return ItHelper<namespace_iterator>(namespace_begin(), namespace_end()); }
			ItHelper<file_iterator> files() { return ItHelper<file_iterator>(file_begin(), file_end()); }
		};
	}
}