#pragma once

#include <string>
#include <cmath>


namespace clang
{
  namespace metrics
  {
    //! File level code metrics.
    struct FileMetrics
    {
      //! Number of code lines of the file, including empty and comment lines.
      unsigned LOC;

      //! Number of non-empty and non-comment code lines of the file.
      unsigned LLOC;

      //! McCabe's Cyclomatic Complexity (on a file level).
      unsigned McCC;
    };

    //! Class, struct and union level code metrics.
    struct ClassMetrics
    {
      //! Name of the class in a human readable form.
      std::string name;

      //! Number of code lines of the class.
      //! Includes empty and comment lines, as well as local methods. Nested, anonymous and local classes are not included.
      unsigned LOC;

      //! Number of code lines of the class.
      //! Includes empty and comment lines, as well as local methods and anonymous, local, or nested classes.
      unsigned TLOC;

      //! Number of non-empty and non-comment code lines of the class.
      //! Includes the non-empty and non-comment lines of local methods. Nested, anonymous, and local classes are not included.
      unsigned LLOC;

      //! Number of non-empty and non-comment code lines of the class.
      //! Includes the non-empty and non-comment code lines of local methods and anonymous, local, or nested classes.
      unsigned TLLOC;

      //! Number of local (i.e. not inherited) methods in the class.
      //! The methods of nested, anonymous, and local classes are not included.
      unsigned NLM;
    };

    //! Function and method level code metrics.
    struct FunctionMetrics
    {
      //! Name of the function or method in a human readable form.
      std::string name;

      //! Number of code lines of the function.
      //! Includes empty and comment lines. Anonymous and local classes inside the function definition are not included.
      unsigned LOC;

      //! Number of code lines of the method, including empty and comment lines, as well as its anonymous and local classes.
      unsigned TLOC;

      //! Number of non-empty and non-comment code lines of the method.
      //! Anonymous and local classes inside the function definition are not included.
      unsigned LLOC;

      //! Number of non-empty and non-comment code lines of the method.
      //! Includes the non-empty and non-comment lines of anonymous and local classes inside the function definition.
      unsigned TLLOC;

      //! McCabe's Cyclomatic Complexity
      unsigned McCC;

      //! Number of operators according to Halstead calculation.
      unsigned H_Operators;

      //! Number of distinct operators according to Halstead calculation.
      unsigned HD_Operators;

      //! Number of operands according to Halstead calculation.
      unsigned H_Operands;

      //! Number of distinct operands according to Halstead calculation.
      unsigned HD_Operands;

      //! Number of statements
      unsigned NOS;

      //! Nesting Level
      unsigned NL;

      //! Nesting Levele Else-If
      unsigned NLE;

      //! Returns the 'Halstead Calculated Program Length (HCPL)' of the function.
      //! HCPL = n1 * log(n1) + n2 * log(n2), where
      //!  n1: number of distinct operators
      //!  n2: number of distinct operands
      //!  log: binary logarithm function (logarithm to the base 2)
      double HCPL() const
      {
        double n1 = (double)HD_Operators;
        double n2 = (double)HD_Operands;
        return n1 * std::log2(n1) + n2 * std::log2(n2);
      }

      //! Returns the 'Halstead Difficulty (HDIF)' of the function.
      //! HDIF = n1/2 * N2/n2, where
      //!  n1: number of distinct operators
      //!  n2: number of distinct operands
      //!  N2: total number of operands
      double HDIF() const
      {
        double n1 = (double)HD_Operators;
        double n2 = (double)HD_Operands;
        double N2 = (double)H_Operands;
        return n1 / 2.0 * N2 / n2;
      }

      //! Returns the 'Halstead Program Length (HPL)' of the function.
      //! HPL = N1 + N2, where
      //!  N1: total number of operators
      //!  N2: total number of operands
      double HPL() const
      {
        return double(H_Operators + H_Operands);
      }

      //! Returns the 'Halstead Program Vocabulary (HPV)' of the function.
      //! HPV = n1 + n2, where
      //!  n1: number of distinct operators
      //!  n2: number of distinct operands
      double HPV() const
      {
        return double(HD_Operators + HD_Operands);
      }

      //! Returns the 'Halstead Volume (HVOL)' of the function.
      //! HVOL = HPL * log(HPV), where log is the binary logarithm function.
      double HVOL() const
      {
        return HPL() * std::log2(HPV());
      }

      //! Returns the 'Halstead Effort (HEFF)' of the function.
      //! HEFF = HDIF * HVOL.
      double HEFF() const
      {
        return HDIF() * HVOL();
      }

      //! Returns the 'Halstead Number of Delivered Bugs (HNDB)' of the function.
      //! HNDB = pow(HEFF, 2/3) / 3000.
      double HNDB() const
      {
        return std::pow(HEFF(), 2.0 / 3.0) / 3000.0;
      }

      //! Returns the 'Halstead Time Required to Program (HTRP)' of the function.
      //! HTRP = HEFF / 18.
      double HTRP() const
      {
        return HEFF() / 18.0;
      }

    };

    //! Enum level code metrics. Includes both classic and C++ 11 strongly typed enums.
    struct EnumMetrics
    {
      //! Name of the enum in a human readable form.
      std::string name;

      //! Number of code lines of the enum, including empty and comment lines.
      unsigned LOC;

      //! Number of non-empty and non-comment code lines of the enum.
      unsigned LLOC;
    };

    //! Namespace level code metrics.
    struct NamespaceMetrics
    {
      //! Name of the namespace in a human readable form.
      std::string name;

      //! Number of code lines of the namespace, including empty and comment lines; however, its subnamespaces are not included.
      unsigned LOC;

      //! Number of code lines of the namespace, including empty and comment lines, as well as its subnamespaces.
      unsigned TLOC;

      //! Number of non-empty and non-comment code lines of the namespace; however, its subnamespaces are not included.
      unsigned LLOC;

      //! Number of non-empty and non-comment code lines of the namespace, including its subnamespaces.
      unsigned TLLOC;

      //! Number of classes in the namespace; however, the classes of its subnamespaces are not included.
      unsigned NCL;

      //! Number of enums in the namespace; however, the enums of its subnamespaces are not included.
      unsigned NEN;

      //! Number of interfaces in the namespace; however, the interfaces of its subnamespaces are not included.
      unsigned NIN;
    };
  }
}
