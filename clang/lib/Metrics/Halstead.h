#pragma once

#include <clang/AST/AST.h>

#include <typeindex>
#include <type_traits>
#include <iostream>
#include <sstream>

#include "UnifiedCXXOperator.h"


namespace clang
{
  namespace metrics
  {
    namespace detail
    {
      // Class for storing and calculating Halstead complexity metrics.
      class HalsteadStorage final
      {
      private:
        // Halstead operator/operand base class.
        class OpBase
        {
        protected:
          OpBase() = default;
  
        public:
          virtual ~OpBase() {}

          virtual bool isSameAs(const OpBase& other) const
          {
            return isOf(other);
          }

          // Returns the name of the operator/operand for easier identification during debugging.
          virtual std::string getDebugName() const = 0;

          // Returns the runtime type as string - since no typeid() is allowed.
          virtual const char* getType() const = 0;

          bool isOf(const OpBase& other) const
          {
            return !strcmp(getType(), other.getType());
          }
        };

      public:
        class Operator : public OpBase {};
        class Operand : public OpBase {};

      public:
        template<class T, class From> class Derive;

      public:
        //! Add Operator.
        template<class T, class... Args>
        auto add(Args&&... args) -> typename std::enable_if<std::is_base_of<Operator, T>::value, T&>::type
        {
          myOperators.push_back(std::unique_ptr<T>(new T(std::forward<Args>(args)...)));
          return static_cast<T&>(*myOperators.back());
        }

        //! Add Operand.
        template<class T, class... Args>
        auto add(Args&&... args) -> typename std::enable_if<std::is_base_of<Operand, T>::value, T&>::type
        {
          myOperands.push_back(std::unique_ptr<T>(new T(std::forward<Args>(args)...)));
          return static_cast<T&>(*myOperands.back());
        }

        //! Print operators for debugging.
        void dbgPrintOperators() const
        {
          std::cout << "\tOperators:\n";
          dbgPrint(myOperators);
        }

        //! Print operands for debugging.
        void dbgPrintOperands() const
        {
          std::cout << "\tOperands:\n";
          dbgPrint(myOperands);
        }

        unsigned getOperatorCount() const
        {
          return myOperators.size();
        }

        unsigned getOperandCount() const
        {
          return myOperands.size();
        }

        unsigned getDistinctOperatorCount() const
        {
          return getDistinctCount(myOperators);
        }

        unsigned getDistinctOperandCount() const
        {
          return getDistinctCount(myOperands);
        }

      private:
        // Helper function.
        template<class T> unsigned getDistinctCount(const T& container) const
        {
          unsigned count = 0;
          for (auto it = container.begin(); it != container.end(); ++it)
          {
            for (auto it2 = it + 1; it2 != container.end(); ++it2)
            {
              if ((*it)->isSameAs(**it2))
                goto OUT_BREAK;
            }
            ++count;

          OUT_BREAK:;
          }
          return count;
        }

        // Internal function used for printing debug info.
        template<class T> void dbgPrint(const T& container) const
        {
          if (container.empty())
            return;

          std::vector<std::string> names;
          names.reserve(container.size());
          for (auto& ptr : container)
            names.push_back(ptr->getDebugName());

          std::sort(names.begin(), names.end());

          unsigned cntr = 1;
          const std::string* prev = &names[0];
          for (size_t i = 1; i < names.size(); ++i)
          {
            if (*prev != names[i])
            {
              std::cout << "\t\t" << cntr << " x " << *prev << '\n';
              cntr = 1;
              prev = &names[i];
            }
            else
            {
              ++cntr;
            }
          }

          std::cout << "\t\t" << cntr << " x " << *prev << '\n';
        }

        // Contains all the matched operators.
        std::vector<std::unique_ptr<Operator>> myOperators;

        // Contains all the matched operands.
        std::vector<std::unique_ptr<Operand>> myOperands;
      };

      // Helper class
      template<class T, class From> class HalsteadStorage::Derive : public From
      {
      public:
        Derive(const T* data) : pMyData(data)
        {}

        const T* getData() const { return pMyData; }

      protected:
        const T* pMyData;
      };



      // Specific operator and operand definitions
      namespace Halstead
      {
        //Macro hack for ease of use: By default, the name of the operators will be printed when calling dbgPrint functions.
      #define HALSTEAD_DERIVE_BASE(OpName, ClangT, From) \
        namespace detail \
        { \
          namespace OpName \
          { \
            class Derive : public HalsteadStorage::Derive<clang::ClangT, HalsteadStorage::From> \
            { \
            protected: \
              using HalsteadStorage::Derive<clang::ClangT, HalsteadStorage::From>::Derive; \
              std::string getDebugName() const override { return #OpName; } \
              const char* getType() const override final { return #OpName; } \
            }; \
          } \
        } \
        class OpName final : public detail::OpName::Derive


      #define HALSTEAD_AUTODERIVE_BASE(OpName, From) \
        namespace detail \
        { \
          namespace OpName \
          { \
            class Derive : public HalsteadStorage::From \
            { \
            private: \
              std::string getDebugName() const override { return #OpName; } \
              const char* getType() const override final { return #OpName; } \
            }; \
          } \
        } \
        class OpName final : public detail::OpName::Derive {}

      #define HALSTEAD_MANUAL_DERIVE_BASE(OpName, From) \
        namespace detail \
        { \
          namespace OpName \
          { \
            class Derive : public HalsteadStorage::From \
            { \
            private: \
              const char* getType() const override final { return #OpName; } \
            }; \
          } \
        } \
        class OpName final : public detail::OpName::Derive

      #define HALSTEAD_DERIVE(OpName, ClangT) HALSTEAD_DERIVE_BASE(OpName, ClangT, Operator)
      #define HALSTEAD_MANUAL_DERIVE(OpName) HALSTEAD_MANUAL_DERIVE_BASE(OpName, Operator)
      #define HALSTEAD_AUTODERIVE(OpName) HALSTEAD_AUTODERIVE_BASE(OpName, Operator)
      #include "HalsteadOperators.h"
      #undef HALSTEAD_DERIVE
      #undef HALSTEAD_MANUAL_DERIVE
      #undef HALSTEAD_AUTODERIVE

      #define HALSTEAD_DERIVE(OpName, ClangT) HALSTEAD_DERIVE_BASE(OpName, ClangT, Operand)
      #define HALSTEAD_MANUAL_DERIVE(OpName) HALSTEAD_MANUAL_DERIVE_BASE(OpName, Operand)
      #define HALSTEAD_AUTODERIVE(OpName) HALSTEAD_AUTODERIVE_BASE(OpName, Operand)
      #include "HalsteadOperands.h"
      #undef HALSTEAD_DERIVE
      #undef HALSTEAD_MANUAL_DERIVE
      #undef HALSTEAD_AUTODERIVE

      #undef HALSTEAD_AUTODERIVE_BASE
      #undef HALSTEAD_DERIVE_BASE
      #undef HALSTEAD_MANUAL_DERIVE_BASE
      }
    }
  }
}