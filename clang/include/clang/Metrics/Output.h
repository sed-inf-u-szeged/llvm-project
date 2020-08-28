#pragma once

#include "Metrics.h"
#include "UIDFactory.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace clang
{
  class FunctionDecl;
  class CXXRecordDecl;
  class EnumDecl;
  class NamespaceDecl;
  class SourceManager;

  namespace metrics
  {
    namespace detail
    {
      class ClangMetrics;
      class GlobalMergeData;
    }

    /*!
     *  The output is stored in an object of this class.
     *  After each source operation, the calculated metrics are merged into the Output object.
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

      //! Access the TU metrics for the specified filename.
      //!  \return pointer to the metrics or nullptr if there are no recorded metrics for this TU
      const FileMetrics* getTranslationUnitMetrics(const std::string& file) const
      {
        auto it = myTranslationUnitMetrics.find(file);
        if (it != myTranslationUnitMetrics.end())
          return &it->second;

        return nullptr;
      }

      //! Returns a reference to the internal UIDFactory received on construction.
      UIDFactory& getFactory() const
      {
        return rMyFactory;
      }

    private:
      // As the final step of the calculation, metrics are aggregated into the Output.
      // This allows private access for the operation without being visible from outside.
      friend class detail::GlobalMergeData;
      friend class detail::ClangMetrics;

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
      std::unordered_map<std::string, FileMetrics> myTranslationUnitMetrics;

    public:
      // Iterator types
      typedef decltype(myFunctionMetrics)::const_iterator        function_iterator;
      typedef decltype(myClassMetrics)::const_iterator           class_iterator;
      typedef decltype(myEnumMetrics)::const_iterator            enum_iterator;
      typedef decltype(myNamespaceMetrics)::const_iterator       namespace_iterator;
      typedef decltype(myFileMetrics)::const_iterator            file_iterator;
      typedef decltype(myTranslationUnitMetrics)::const_iterator tu_iterator;

      //! Returns a const ForwardIterator to the beginning of the internal function metrics structure.
      //! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, FunctionMetrics>.
      //! The order of iteration is undefined, but all elements are guaranteed to be iterated over until function_end() is reached.
      function_iterator function_begin() const { return myFunctionMetrics.begin(); }

      //! Returns a const ForwardIterator to the end of the internal function metrics structure.
      //! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, FunctionMetrics>.
      function_iterator function_end() const { return myFunctionMetrics.end(); }

      //! Returns a const ForwardIterator to the beginning of the internal class metrics structure.
      //! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, ClassMetrics>.
      //! The order of iteration is undefined, but all elements are guaranteed to be iterated over until class_end() is reached.
      class_iterator class_begin() const { return myClassMetrics.begin(); }

      //! Returns a const ForwardIterator to the end of the internal class metrics structure.
      //! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, ClassMetrics>.
      class_iterator class_end() const { return myClassMetrics.end(); }

      //! Returns a const ForwardIterator to the beginning of the internal enum metrics structure.
      //! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, EnumMetrics>.
      //! The order of iteration is undefined, but all elements are guaranteed to be iterated over until enum_end() is reached.
      enum_iterator enum_begin() const { return myEnumMetrics.begin(); }

      //! Returns a const ForwardIterator to the end of the internal enum metrics structure.
      //! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, EnumMetrics>.
      enum_iterator enum_end() const { return myEnumMetrics.end(); }

      //! Returns a const ForwardIterator to the beginning of the internal namespace metrics structure.
      //! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, NamespaceMetrics>.
      //! The order of iteration is undefined, but all elements are guaranteed to be iterated over until namespace_end() is reached.
      namespace_iterator namespace_begin() const { return myNamespaceMetrics.begin(); }

      //! Returns a const ForwardIterator to the end of the internal namespace metrics structure.
      //! The iterator's value_tpye is std::pair<std::shared_ptr<const UID>, NamespaceMetrics>.
      namespace_iterator namespace_end() const { return myNamespaceMetrics.end(); }

      //! Returns a const ForwardIterator to the beginning of the internal file metrics structure.
      //! The iterator's value_tpye is std::pair<std::string, FileMetrics>.
      //! The order of iteration is undefined, but all elements are guaranteed to be iterated over until file_end() is reached.
      file_iterator file_begin() const { return myFileMetrics.begin(); }

      //! Returns a const ForwardIterator to the end of the internal file metrics structure.
      //! The iterator's value_tpye is std::pair<std::string, FileMetrics>.
      file_iterator file_end() const { return myFileMetrics.end(); }

      //! Returns a const ForwardIterator to the beginning of the internal tranlsation unit metrics structure.
      //! The iterator's value_tpye is std::pair<std::string, FileMetrics>.
      //! The order of iteration is undefined, but all elements are guaranteed to be iterated over until tu_end() is reached.
      tu_iterator tu_begin() const { return myTranslationUnitMetrics.begin(); }

      //! Returns a const ForwardIterator to the end of the internal translation unit metrics structure.
      //! The iterator's value_tpye is std::pair<std::string, FileMetrics>.
      tu_iterator tu_end() const { return myTranslationUnitMetrics.end(); }


      //! Iterator wrappers for use in range based for loops.
      ItHelper<function_iterator> functions() const { return ItHelper<function_iterator>(function_begin(), function_end()); }
      ItHelper<class_iterator> classes() const { return ItHelper<class_iterator>(class_begin(), class_end()); }
      ItHelper<enum_iterator> enums() const { return ItHelper<enum_iterator>(enum_begin(), enum_end()); }
      ItHelper<namespace_iterator> namespaces() const { return ItHelper<namespace_iterator>(namespace_begin(), namespace_end()); }
      ItHelper<file_iterator> files() const { return ItHelper<file_iterator>(file_begin(), file_end()); }
      ItHelper<tu_iterator> translation_units() const { return ItHelper<tu_iterator>(tu_begin(), tu_end()); }
    };
  }
}
