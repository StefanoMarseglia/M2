-- -*- coding: utf-8 -*-
--- status: DRAFT
--- author(s): last edit: MES
--- notes: 

undocumented {
	  (generators, EngineRing),
	  (generators, FractionField),
	  (generators, GaloisField),
	  (generators, PolynomialRing),
	  (generators, QuotientRing),
	  (generators, InexactField)}

document {
     Key => generators,
     Headline => "provide matrix or list of generators",
     Usage => "generators x\ngens x",
     Inputs => { "x",
	  CoefficientRing => {"only used if ", TT "x", " is a ring"}  },
     Outputs => { {"provides the generators of ", TT "x", 
	       " in a convenient form, as a list or a matrix, depending on the type"} },
     PARA{},
     "Produces the generators of a Gröbner basis, a polynomial ring, an ideal,
     a free module, a free group, a submodule given by
     means of generators (or for which generators have been computed),
     or a free monoid.",
     PARA{},
     "Usually the result is a list of generators, but the generators of
     a module or Gröbner basis are provided as the columns in a matrix.  
     The matrix is stored in a module M under M.generators, unless the matrix
     is the identity matrix.",
     PARA{},
     "The symbol ", TT "gens", " is a synonym for ", TT "generators", ".",     
     SeeAlso => {numgens, Monoid, GroebnerBasis, Module, relations, subquotient}
     }

document {
     Key => {generator,(generator,Ideal),(generator,Module)},
     Headline => "provide a single generator",
     Usage => "generator I",
     Inputs => { "I" => {ofClass{Ideal,Module}}},
     Outputs => {{"the single generator of ", TT "I", ", if it has just one"}},
     PARA {"If the number of apparent generators is greater than 1, then ", TO "trim", " will be called."},
     EXAMPLE lines ///
     	  I = ideal (4,6)
	  generator I
	  M = image matrix {{4,6},{0,0}}
	  generator M
     ///,
     SeeAlso => {generators}
     }

document { 
     Key => (generators,GroebnerBasis),
     Headline => "the generator matrix of a Gröbner basis",
     Usage => "generators g\ngens g",
     Inputs => { "g",	  
	  CoefficientRing => "unused option" },
     Outputs => {Matrix => {"whose columns are the generators of the Gröbner basis ", TT "g"}},
     "The following ideal defines a set of 18 points over the complex numbers.  We compute a
     lexicographic Gröbner basis of the ideal.",
     EXAMPLE {
	  "R = QQ[a..d, MonomialOrder=>Lex];",
	  "I = ideal(a^7-b-3, a*b-1, a*c^2-3, b*d-4);",
	  "gens gb I"
	  },
     SeeAlso => {"Gröbner bases"}
     }
document {
     Key => (generators,Module),
     Headline => "the generator matrix of a module",     
     Usage => "generators M\ngens M",
     Inputs => { "M",
	  CoefficientRing => "unused option" 
	  },
     Outputs => {
	  {"the matrix of generators of ", TT "M", "."}
	  },
     "Every module in Macaulay2 has, at least implicitly, a generator matrix and a 
     matrix of relations, both of which are matrices between free modules.  
     This function returns the generator matrix.  The module is generated by (the images of) the
     columns of this matrix.",
     EXAMPLE {
	  "R = GF(8,Variable=>a)",
      	  "f = R_0 ++ R_0^2 ++ R_0^3 ++ R_0^4",
      	  "generators image f",
      	  "generators cokernel f"
	  },
     Caveat => {
	  "This function returns a matrix with the given generators.  This 
	  set of generators may not be minimal, or sorted in any particular 
	  order. Use ", TO (trim,Module), " or ", TO (mingens,Module), " instead."
	  },
     SeeAlso => {(relations,Module)}
     }
document { 
     Key => (generators,GeneralOrderedMonoid),
     Headline => "list of generators",
     Usage => "generators M\ngens M",
     Inputs => {
	  "M",
	  CoefficientRing => "unused option"
	  },
     Outputs => {
	  List => "of generators"
	  },
     EXAMPLE lines ///
     	  R = QQ[a..d];
	  M = monoid R
	  gens M
	  ///,
     SeeAlso => {monoid}
     }
document {
     Key => {(generators, Ideal),
	  (generators, MonomialIdeal)},
     Headline => "the generator matrix of an ideal",
     Usage => "generators I\ngens I",
     Inputs => {"I",
	  CoefficientRing => "unused option"
	  },
     Outputs => { Matrix => {"the one-row matrix whose entries are the generators of ", TT "I"} },
     "Each ideal in ", EM "Macaulay2", " comes equipped with a one-row
     matrix with the generators of the ideal.
     It is this matrix which is returned.",
     EXAMPLE {
	  "R = ZZ/101[a,b,c];",
      	  "I = ideal(a^2,a*b-2,c^4,a*c-1,a*c-1)",
      	  "generators I"
	  },
     "To obtain a list of generators, rather than a matrix, use",
     EXAMPLE {
	  "first entries generators I"
	  },
     "If you want to remove unnecessary generators, use ", TO trim, ".",
     EXAMPLE {
	  "I = trim I",
	  "gens I"
	  }
     }
document { 
     Key => {(generators,Ring),
	  [generators,CoefficientRing]
	  },
     Headline => "the list of generators of a ring",
     Usage => "generators R\ngens R",
     Inputs => {
	  "R",
	  CoefficientRing => Ring => {"the coefficient ring over which to provide the generators of ", TT "R", ", if different
	       from the most recent one"}
	  },
     Outputs => {
	  List => "of generators"
	  },
     EXAMPLE {
	  "A = QQ[x,y];",
	  "gens A",
	  "kk = toField(QQ[t]/(t^3-t-1));",
	  "B = kk[x,y,z];",
	  "generators B",
	  "generators(B, CoefficientRing => QQ)"
	  }
     }