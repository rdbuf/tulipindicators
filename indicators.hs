{-# LANGUAGE GADTs #-}

import Data.List

type Input = String
type Output = String
type Option = String
type Name = String
type Abbrev = String
type OutputLen = String
type Constraint = String

data Indicator where
    Indicator :: (Name, Abbrev) -> [Input] -> [Option] -> [Constraint] -> OutputLen -> [Output] -> Indicator

endl = "\n"
indent :: Int -> String -> String
indent n string = replicate n ' ' <> string

signature ret name args = ret <> " " <> name <> "(" <> intercalate ", " args <> ")"
body lines = [" {"] <> map (indent 4) lines <> ["}"]

joinlines :: Semigroup a => [a] -> [a] -> [a]
joinlines [] b = b
joinlines a [] = a
joinlines a b = init a <> [last a <> head b] <> tail b

statement :: String -> String
statement thing = thing <> ";"
at thing idx = thing <> "[" <> idx <> "]"
assign lhs rhs = lhs <> " = " <> rhs
ifthen cond lines = joinlines ["if (" <> cond <> ")"] (body lines)
not_ expr = "!(" <> expr <> ")"

header = "/* \n\
\ * Tulip Indicators \n\
\ * https://tulipindicators.org/ \n\
\ * Copyright (c) 2010-2017 Tulip Charts LLC \n\
\ * Lewis Van Winkle (LV@tulipcharts.org) \n\
\ * \n\
\ * This file is part of Tulip Indicators. \n\
\ * \n\
\ * Tulip Indicators is free software: you can redistribute it and/or modify it \n\
\ * under the terms of the GNU Lesser General Public License as published by the \n\
\ * Free Software Foundation, either version 3 of the License, or (at your \n\
\ * option) any later version. \n\
\ * \n\
\ * Tulip Indicators is distributed in the hope that it will be useful, but \n\
\ * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or \n\
\ * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License \n\
\ * for more details. \n\
\ * \n\
\ * You should have received a copy of the GNU Lesser General Public License \n\
\ * along with Tulip Indicators.  If not, see <http://www.gnu.org/licenses/>. \n\
\ * \n\
\ */ \n\
\ \n\
\#include \"../indicators.h\""

genStub (Indicator (_, abbrev) inputs options constraints outLen outputs) = let
    start = joinlines [signature "int" ("ti_" <> abbrev <> "_start") ["TI_REAL const *options"]] $ body (opts <> [statement $ "return " <> outLen])
    sig = [signature "int" ("ti_" <> abbrev) ["int size", "TI_REAL const *const *inputs", "TI_REAL const *options", "TI_REAL *const *outputs"]]
    unpack source type_ idx name = statement $ assign (type_ <> name) (at source (show idx))
    opts = zipWith (unpack "options" "const TI_REAL ") [0..] options
    ins = zipWith (unpack "inputs" "const TI_REAL *") [0..] inputs
    outs = zipWith (unpack "outputs" "TI_REAL *") [0..] outputs
    genconstr constraint = ifthen (not_ constraint) [statement "return TI_INVALID_OPTION"]
    constrs = concat . map genconstr $ constraints
    epilogue = map statement ["assert(output - outputs[0] == size - ti_rmta_start(options))", "return TI_OKAY"]
    main = joinlines sig $ body (opts <> ins <> outs <> [endl] <> constrs <> [endl, "#error CODE GOES HERE", endl] <> epilogue)
    in intercalate endl $ [header] <> [endl] <> start <> [endl] <> main

main = putStrLn . genStub $ Indicator ("Acceleration Bands", "abands") ["high", "low", "real"] ["period", "factor"] ["period >= 1"] "(int)period-1" ["abands"]

-- Todo:
-- - indicators.h
-- - indicators_index.c
-- - test suite
-- - ux
-- - better abstractions