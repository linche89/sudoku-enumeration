--
-- !!! OBSOLETE !!! OBSOLETE !!!
-- use sudoku_equiv.cc instead!
--
-- Quick and dirty hack for more symmetry elimination for the Sudoku counting
-- problem. The code is inefficient but gets the job done in a reasonable
-- amount of time.
--
-- This turned out to be inadequate.
--
-- This is a Haskell program (see haskell.org)
--
-- Author: Bertram Felgenhauer
-- History:
-- 2005-05-23: Initial version.
--
-- It's based on a hint by A F Jarvis in a personal email.
--
-- result: divides the 36288 possible configurations of boxes 2, 3 into
--         1089 equivalence classes
-- after implementing some more of afj's ideas, 306 equivalence classes remain

import List

-- I was lazy: permutations code is from
-- http://www.haskell.org/pipermail/haskell-cafe/2002-June/003122.html
-- with a small bug fix.

selections :: [a] -> [(a,[a])]
selections []     = []
selections (x:xs) = (x, xs) : [ (y, x:ys) | (y, ys) <- selections xs ]

permutations :: [a] -> [[a]]
permutations [] = [[]]
permutations xs = [ y:zs | (y, ys) <- selections xs, zs <- permutations ys ]

overlap :: Eq a => [a] -> [a] -> Bool
overlap a b = or [ any (x==) b | x <- a ]

-- generate a list of box 2 and 3 configurations, normalized
-- by the first column.

box23 :: [[[Int]]]
box23 = [ [p1, p2, p3] |
          p1 <- permutations [4, 5, 6, 7, 8, 9],
          p1!!0 < p1!!1, p1!!1 < p1!!2,
          p1!!3 < p1!!4, p1!!4 < p1!!5,
          p1!!0 < p1!!3,
          p2 <- permutations [1, 2, 3, 7, 8, 9],
	  not $ overlap (take 3 p1) (take 3 p2),
	  not $ overlap (drop 3 p1) (drop 3 p2),
          p3 <- permutations [1, 2, 3, 4, 5, 6],
	  not $ overlap (take 3 p1) (take 3 p3),
	  not $ overlap (drop 3 p1) (drop 3 p3),
	  not $ overlap (take 3 p2) (take 3 p3),
	  not $ overlap (drop 3 p2) (drop 3 p3) ]

-- transpose :: [[a]] -> [[a]]
-- transpose = foldr (zipWith (:)) (repeat [])

{-
> Hmm, I guess you're exploiting the 6!^2 possible renamings of the
> digits 1-9 that leave the first box unchanged up to a permutation
> of rows and a permutation of columns here.  I wonder if there's a
> good way to incorporate that into my program directly.  Food for
> thought...
-}

-- normalize a given configuration of boxes 2 and 3:
-- new: also consider choosing a different block as the first block and
-- rename accordingly
-- if boxes contain a rectangle ab, also consider ba as equivalent
--                              ba                ab
-- first, adjoin box 1, then apply all the possible renamings mentioned above,
-- then reorder the rows according to box 1, then order box 2 columns and
-- box 3 columns and then the two boxes; finally take the smallest of those
-- according to lexicographic order.

{-
normalize :: [[Int]] -> [[Int]]
normalize pi = (sort $ map norm_ $
    [ [ [ maybe (-1) id $ lookup x ren | x <- p ] |
        p <- zipWith (++) [[1, 2, 3], [4, 5, 6], [7, 8, 9]] pi ] |
      ren <- renamings ] ) !! 0 where

    renamings :: [[(Int, Int)]]
    renamings = [ zip [1..9] $ concat x |
        x <- concat $ map (permutations.transpose) $
             permutations [[1, 4, 7], [2, 5, 8], [3, 6, 9]] ]

    norm_ :: [[Int]] -> [[Int]]    
    norm_ pi = let q = transpose $ map (drop 3) $ sort pi in
        transpose $ concat $ sort [sort $ take 3 q, sort $ drop 3 q]
-}

ren0 :: [[Int]] -> [[[Int]]]
ren0 p = let pp = [[[1, 2, 3], [4, 5, 6], [7, 8, 9]],
                   map (take 3) p, map (drop 3) p] in
    [ [ [ maybe (-1) id $ lookup x (zip (concat a) [1..9]) | x <- l ] |
        l <- zipWith (++) b c ] | (a, [b, c]) <- selections pp ]

ren1 :: [[Int]] -> [[[Int]]]
ren1 pp = ren1_ pp 0 0 1 1 where
  ren1_ pp 5  _  _  _  = [pp]
  ren1_ pp x1 2  _  _  = ren1_ pp (x1+1) 0 (x1+2) 1
  ren1_ pp x1 y1 6  _  = ren1_ pp x1 (y1+1) (x1+1) (y1+1)
  ren1_ pp x1 y1 x2 3  = ren1_ pp x1 y1 (x2+1) (y1+1)
  ren1_ pp x1 y1 x2 y2 = if pp!!y1!!x1 == pp!!y2!!x2 &&
  	      	       	    pp!!y2!!x1 == pp!!y1!!x2 then
                            ren1_ pp x1 y1 x2 (y2+1) ++
			    ren1_ (replace pp) x1 y1 x2 (y2+1)
                          else
                            ren1_ pp x1 y1 x2 (y2+1) where
    replace pp = take y1 pp ++
                 [ take x1 (pp!!y1) ++
                   [pp!!y2!!x1] ++
                   (take (x2-x1-1) $ drop (x1+1) (pp!!y1)) ++
                   [pp!!y1!!x1] ++
                   (take (6-1-x2) $ drop (x2+1) (pp!!y1)) ] ++
                 (take (y2-y1-1) $ drop (y1+1) pp) ++
                 [ take x1 (pp!!y2) ++
                   [pp!!y1!!x1] ++
                   (take (x2-x1-1) $ drop (x1+1) (pp!!y2)) ++
                   [pp!!y2!!x1] ++
                   (take (6-1-x2) $ drop (x2+1) (pp!!y2)) ] ++
		 (take (3-1-y2) $ drop (y2+1) pp)

normalize :: [[Int]] -> [[Int]]
normalize pi = (sort $ equiv pi) !! 0

equiv pi = map norm_ $
    [ [ [ maybe (-1) id $ lookup x ren | x <- p ] |
        p <- zipWith (++) [[1, 2, 3], [4, 5, 6], [7, 8, 9]] pii ] |
      ren <- renamings,
      pi' <- ren0 pi,
      pii <- ren1 pi' ] where

    renamings :: [[(Int, Int)]]
    renamings = [ zip [1..9] $ concat x |
        x <- concat $ map (permutations.transpose) $
             permutations [[1, 4, 7], [2, 5, 8], [3, 6, 9]] ]

    norm_ :: [[Int]] -> [[Int]]    
    norm_ pi = let q = transpose $ map (drop 3) $ sort pi in
        transpose $ concat $ sort [sort $ take 3 q, sort $ drop 3 q]


-- generate raw list: (1.3s)
-- main = sequence $ map print $ box23
-- generate corresponding normalized list: 
main = sequence $ map print $ map normalize $ box23
