/* Author: Ed Russell */
/* sudoku solutions counter, v2
 *
 * Results (equiv class index, equiv class representative,
 * nr. normalised box 1,2,3s in class, nr solutions for
 * any particular box in the class) are as follows:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  1 : 124 357 689 125 367 489 : 2484 :   97961464
  2 : 124 357 689 125 368 479 : 2592 :   97539392
  3 : 124 357 689 125 369 478 : 1296 :   98369440
  4 : 124 357 689 125 378 469 : 1512 :   97910032
  5 : 124 357 689 126 348 579 : 2808 :   96482296
  6 : 124 357 689 126 349 578 :  684 :   97549160
  7 : 124 357 689 126 357 489 : 1512 :   97287008
  8 : 124 357 689 126 358 479 : 1944 :   97416016
  9 : 124 357 689 126 359 478 : 2052 :   97477096
 10 : 124 357 689 127 348 569 :  288 :   96807424
 11 : 124 357 689 127 358 469 :  864 :   98119872
 12 : 124 357 689 128 347 569 : 1188 :   98371664
 13 : 124 357 689 128 357 469 :  648 :   98128064
 14 : 124 357 689 128 369 457 : 2592 :   98733568
 15 : 124 357 689 135 269 478 :  648 :   97455648
 16 : 124 357 689 135 278 469 :  360 :   97372400
 17 : 124 357 689 136 259 478 : 3240 :   97116296
 18 : 124 357 689 138 267 459 :  540 :   95596592
 19 : 124 357 689 138 269 457 :  756 :   97346960
 20 : 124 357 689 145 269 378 :  324 :   97714592
 21 : 124 357 689 145 278 369 :  432 :   97992064
 22 : 124 357 689 146 239 578 :  756 :   98153104
 23 : 124 357 689 147 269 358 :  864 :   98733184
 24 : 124 357 689 148 269 357 :  108 :   98048704
 25 : 124 357 689 156 239 478 :  756 :   96702240
 26 : 124 358 679 125 368 479 :  516 :   98950072
 27 : 124 358 679 126 348 579 :  576 :   97685328
 28 : 124 358 679 127 358 469 :  432 :   98784768
 29 : 124 358 679 137 269 458 :  324 :   98493856
 30 : 124 358 679 147 258 369 :   72 :  100231616
 31 : 124 358 679 147 269 358 :  216 :   99525184
 32 : 124 358 679 156 237 489 :  252 :   96100688
 33 : 124 359 678 127 356 489 :  288 :   96631520
 34 : 124 359 678 127 359 468 :  864 :   97756224
 35 : 124 359 678 147 258 369 :  216 :   99083712
 36 : 124 359 678 147 268 359 :  432 :   98875264
 37 : 124 369 578 125 369 478 :  216 :  102047904
 38 : 124 369 578 127 369 458 :  144 :  101131392
 39 : 124 369 578 135 267 489 :  324 :   96380896
 40 : 124 369 578 147 258 369 :  108 :  102543168
 41 : 124 379 568 146 239 578 :   12 :   99258880
 42 : 126 348 579 135 249 678 :   20 :   94888576
 43 : 126 378 459 147 258 369 :   24 :   97282720
 44 : 147 258 369 147 258 369 :    4 :  108374976
Grand total = 0x0339a6ca6180 * 362880 * 5184 solutions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 * ... which is 6670903752021072936960 in total.
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define NREP  36288
#define NREPX 22266

#define SWAP(A,B) { int t = A; A = B; B = t; }

typedef unsigned int uint32;

static int rep[NREP][7];
static int colour[NREP];

static uint32 tmpl[9][16];
static int    ntmpl[9];

static int    grand_total_hi, grand_total_lo;

/* ----------------------------------------------------------------------
 * miscellaneous utility functions
 */

static int pack(int a, int b, int c) {
  int packme[10]={0}, packed=0, i;
  packme[a] = packme[b] = packme[c] = 1;
  for (i=1; i<=9; i++) if (packme[i]) packed = (packed<<4) + i;
  return packed;
}

static int qsrepcmp(const void *a, const void *b) {
  return memcmp(a,b,6*sizeof(int));
}

static void reporder(int *rep) {
  if (rep[0] > rep[1]) SWAP(rep[0],rep[1]);
  if (rep[0] > rep[2]) SWAP(rep[0],rep[2]);
  if (rep[1] > rep[2]) SWAP(rep[1],rep[2]);
  if (rep[3] > rep[4]) SWAP(rep[3],rep[4]);
  if (rep[3] > rep[5]) SWAP(rep[3],rep[5]);
  if (rep[4] > rep[5]) SWAP(rep[4],rep[5]);
  if (rep[0]*0x1000+rep[1] > rep[3]*0x1000+rep[4]) {
    SWAP(rep[0],rep[3]);
    SWAP(rep[1],rep[4]);
    SWAP(rep[2],rep[5]);
  }
  return;
}

static void printrep(int *rep) {
  printf("%3x %3x %3x %3x %3x %3x",rep[0],rep[1],rep[2],rep[3],rep[4],rep[5]);
  return;
}

static void replistsort(int (*replist)[7], int nrep) {
  int i, j;
  qsort(replist,nrep,7*sizeof(int),qsrepcmp);
  for (i=j=1; i<nrep; i++) {
    if (qsrepcmp(replist+i-1,replist+i)) {
      if (j < i) memcpy(replist+j,replist+i,7*sizeof(int));
      j++;
    } else {
      replist[j-1][6] += replist[i][6];
    }
  }
  return;
}

static int findrep(int *findme) {
  int i0 = -1;
  int i1 = NREPX;
  while (i1 > i0+1) {
    int imid = (i0+i1) / 2;
    int res  = memcmp(findme,rep+imid,6*sizeof(int));
    if   (res == 0) return imid;
    else if (res < 0) i1 = imid;
    else              i0 = imid;
  }
  return -1;
}

/* ----------------------------------------------------------------------
 * Get full representatives list of normalised boxes 1,2,3.  Reps are
 * stored as 6 ints s.t. rep[0]<rep[1]<rep[2], rep[3]<rep[4]<rep[5] and
 * rep[0,1,2] < rep[3,4,5], plus one further int to count the number of
 * normalised box 1,2,3s that map to this rep.
 */

static void get_repx(int level) {
  static int box[3][9];
  static int ct;
  int        ii, jj, ok;
  int        i;

  if (level == 0) for (i=ct=0; i<9; i++) box[i/3][i%3] = i + 1;

  /* enforce row ordering
   */
  if (level==2 && box[0][4]<box[0][3]) return;
  if (level==3 && box[0][5]<box[0][4]) return;
  if (level==4 && box[0][6]<box[0][3]) return;
  if (level==5 && box[0][7]<box[0][6]) return;
  if (level==6 && box[0][8]<box[0][7]) return;

  /* store the representative when we've got normalised boxes 1,2,3
   */
  if (level == 18) {
    int tmp[6];
    for (i=3; i<9; i++) tmp[i-3] = pack(box[0][i],box[1][i],box[2][i]);
    reporder(tmp);
    memcpy(rep+ct,tmp,6*sizeof(int));
    rep[ct++][6] = 1;
    return;
  }

  /* fall-through to recursive box-filling code */

  ii = level / 6;
  jj = 3 + level%6;
  ok = 0x3fe;

  for (i=0; i<jj; i++) ok &= ~(1<<box[ii][i]);
  for (i=0; i<3*ii+jj%3; i++) ok &= ~(1<<box[i/3][i%3+3*(jj/3)]);
  for (i=1; i<=9; i++) {
    if (ok & (1<<i)) {
      box[ii][jj] = i;
      get_repx(level+1);
    }
  }

  /* finish up by sorting & deduping the replist
   */
  if (level == 0) replistsort(rep,NREP);

  return;
}

/* ----------------------------------------------------------------------
 * Find representatives equivalent to a given one and relabel all
 * of these with the given "colour"
 */

static void permute(int *rep, int true_colour) {
  int box[3][9];
  int perm[18] = { 0,1,2,  0,2,1,  1,0,2,  1,2,0,  2,0,1,  2,1,0 };
  int ordbox, *colperm, *cellperm[3];
  int map[10];
  int i;

  /* build basic box from representatives (but don't worry about
   * digit order within columns)
   */
  for (i=0; i<9; i++) box[i/3][i%3] = i + 1;
  for (i=0; i<6; i++) {
    box[0][i+3] =       rep[i]>>8;
    box[1][i+3] = 15 & (rep[i]>>4);
    box[2][i+3] = 15 &  rep[i];
  }

  /* find list of equivalent representatives
   */
  for (ordbox=0; ordbox<3; ordbox++) {
    for (colperm=perm; colperm<perm+18; colperm+=3) {
      for (cellperm[0]=perm; cellperm[0]<perm+18; cellperm[0]+=3) {
        for (cellperm[1]=perm; cellperm[1]<perm+18; cellperm[1]+=3) {
          for (cellperm[2]=perm; cellperm[2]<perm+18; cellperm[2]+=3) {
            int newbox[3][9];
            int tmp[6];

            /* relabel digits so that box {ordbox} has value 1+x+3*y
             * in row {cellperm[x][y]} column {colperm[x]}
             */
            for (i=0; i<9; i++) {
              int x = i % 3;
              int y = i / 3;
              map[ box[ cellperm[x][y] ][ 3*ordbox+colperm[x] ] ] = i + 1;
            }
            for (i=0; i<27; i++)
              newbox[i/9][i%9] = map[box[i/9][i%9]];

            /* construct the representative for this relabelled arrangement
             */
            for (i=0; i<6; i++) {
              int col = 3*((ordbox+1+(i/3))%3) + i%3;
              tmp[i] = pack(newbox[0][col],newbox[1][col],newbox[2][col]);
            }
            reporder(tmp);

            /* this new rep may not correspond to any normalised box123;
             * but if it does then we should relabel that matching rep
             */
            if ( (i=findrep(tmp)) >= 0) colour[i] = true_colour;
          }
        }
      }
    }
  }

  return;
}

/* ----------------------------------------------------------------------
 * Find all solutions, given boxes 1,2,3, such that row4 < row5 < row6
 * and row7 < row8 < row9 and row4 < row7.  The user should then multiply
 * the result by 72 to get the unconstrained total number of solutions.
 */

static void solve_rep(int *rep, int level, int multiplier) {
  static int grid[9][9] = {{0}};
  static int xok[6], yok[6];
  static int rep2[NREP][7];
  static int ct, outer_ct;
  int        ii, jj, ok;
  int        i;

  /* entry point: set up the grid
   */
  if (level == 0) {
    for (i=0; i<9; i++) grid[i/3][i%3] = i + 1;
    for (i=0; i<6; i++) {
      xok[i] = 0x3fe ^ (1 << (grid[0][i+3] =       rep[i]>>8 ) ) \
                     ^ (1 << (grid[1][i+3] = 15 & (rep[i]>>4)) ) \
                     ^ (1 << (grid[2][i+3] = 15 &  rep[i]    ) ) ;
      yok[i] = 0x3fe;
    }
    ct = outer_ct = 0;
  }

  /* enforce row ordering
   */
  if (level==2 && grid[4][0]<grid[3][0]) return;
  if (level==3 && grid[5][0]<grid[4][0]) return;
  if (level==4 && grid[6][0]<grid[3][0]) return;
  if (level==5 && grid[7][0]<grid[6][0]) return;
  if (level==6 && grid[8][0]<grid[7][0]) return;

  /* early levels: place boxes 4,7
   */
  if (level < 18) {
    ii = 3 + level%6;
    jj = level / 6;
    ok = 0x3fe;
    for (i=0; i<ii; i++) ok &= ~(1<<grid[i][jj]);
    for (i=0; i<ii%3+3*jj; i++) ok &= ~(1<<grid[i%3+3*(ii/3)][i/3]);
    for (i=1; i<=9; i++) {
      if (ok & (1<<i)) {
        grid[ii][jj] = i;
        yok[ii-3] ^= 1<<i;
        solve_rep(rep,level+1,multiplier);
        yok[ii-3] ^= 1<<i;
      }
    }

    /* end of recursion: tot up the counts
     */
    if (level == 0) {
      printf("%10d\n",ct);
      for (i=0; i<multiplier; i++) {
        grand_total_lo += ct;
        grand_total_hi += grand_total_lo >> 24;
        grand_total_lo &= 0xffffff;
      }
    }
    return;
  }

  /* level 18: progress counter
   */
  if (level == 18) {
    i = 36287 - outer_ct++;
    if (i%100 == 0) {
      fprintf(stderr,"<%d> ",i/=100);
      putc(8,stderr); putc(8,stderr); putc(8,stderr); putc(8,stderr);
      while (i /= 10) putc(8,stderr);
    }
  }

  /* level 18: check that we haven't tackled this rep2 already
   */
  if (level == 18) {
    int tmp[6];
    for (i=3; i<9; i++) tmp[i-3] = pack(grid[i][0],grid[i][1],grid[i][2]);
    reporder(tmp);
    memcpy(rep2[outer_ct-1],tmp,6*sizeof(int));
    rep2[outer_ct-1][6] = 0;
    for (i=0; i<outer_ct-1; i++) if (0 == memcmp(tmp,rep2[i],6*sizeof(int))) break;
    if (i < outer_ct-1) {
      ct += rep2[i][6];
      return;
    }
  }

  /* levels 18-21: place 1s
   */
  if (level < 22) {
    ii = 3*(level/2) - 27;
    jj = 3*(level%2);
    for (i=0; i<9; i++) {
      if (xok[jj+i%3] & yok[ii+i/3] & 2) {
        grid[3+ii+i/3][3+jj+i%3] = 1;
        xok[jj+i%3] ^= 2;
        yok[ii+i/3] ^= 2;
        solve_rep(rep,level+1,multiplier);
        grid[3+ii+i/3][3+jj+i%3] = 0;
        xok[jj+i%3] ^= 2;
        yok[ii+i/3] ^= 2;
      }
    }
  }

  /* level 22: enumerate possibilities for the remaining 32 cells
   */
  if (level == 22) {

    /* part 1: get uint32 templates for digits 2...9
     * (needs to be fast, so we didn't use recursion)
     */
    {
      int pos[6][6];
      int digit;
      for (ii=i=0; ii<6; ii++)
        for (jj=0; jj<6; jj++)
          if (grid[3+ii][3+jj] == 0)
            pos[ii][jj] = i++;
      for (digit=2; digit<=8; digit++) {
        int iposs[4], jposs[4];
        int i1, i2, j1, j2;
        for (ii=i=0; ii<6; ii++) if (yok[ii]&(1<<digit)) iposs[i++] = ii;
        for (jj=i=0; jj<6; jj++) if (xok[jj]&(1<<digit)) jposs[i++] = jj;
        ntmpl[digit] = 0;
        for (i1=0; i1<2; i1++) {
          for (j1=0; j1<2; j1++) {
            if (grid[3+iposs[i1]][3+jposs[j1]] == 0) {
              for (i2=2; i2<4; i2++) {
                if (grid[3+iposs[i2]][3+jposs[j1^1]] == 0) {
                  for (j2=2; j2<4; j2++) {
                    if (  grid[3+iposs[i1^1]][3+jposs[j2  ]]==0 \
                       && grid[3+iposs[i2^1]][3+jposs[j2^1]]==0 ) {
                      tmpl[digit][ntmpl[digit]++] =                      \
                            (((uint32)1)<<pos[iposs[i1  ]][jposs[j1  ]]) \
                          | (((uint32)1)<<pos[iposs[i1^1]][jposs[j2  ]]) \
                          | (((uint32)1)<<pos[iposs[i2  ]][jposs[j1^1]]) \
                          | (((uint32)1)<<pos[iposs[i2^1]][jposs[j2^1]]) ;
                    }
                  } /* j2 */
                }
              } /* i2 */
            }
          } /* j1 */
        }  /* i1 */
      } /* digit */
    }  /* end of template-packing block */

    /* part 2: enumerate solutions
     * (quicker not to use compression for this tiny problem)
     */
    {
      uint32 mask;
      int    i2, i3, i4, i5, i6, i7, i8;
      int    inner_ct;
      for (i2=inner_ct=0; i2<ntmpl[2]; i2++) {
        mask = tmpl[2][i2];
        for (i3=0; i3<ntmpl[3]; i3++) {
          if (0 == (mask&tmpl[3][i3])) {
            mask |= tmpl[3][i3];
            for (i4=0; i4<ntmpl[4]; i4++) {
              if (0 == (mask&tmpl[4][i4])) {
                mask |= tmpl[4][i4];
                for (i5=0; i5<ntmpl[5]; i5++) {
                  if (0 == (mask&tmpl[5][i5])) {
                    mask |= tmpl[5][i5];
                    for (i6=0; i6<ntmpl[6]; i6++) {
                      if (0 == (mask&tmpl[6][i6])) {
                        mask |= tmpl[6][i6];
                        for (i7=0; i7<ntmpl[7]; i7++) {
                          if (0 == (mask&tmpl[7][i7])) {
                            mask |= tmpl[7][i7];
                            for (i8=0; i8<ntmpl[8]; i8++) {
                              if (0 == (mask&tmpl[8][i8])) {
                                inner_ct++;
                              }
                            } /* i8 */
                            mask ^= tmpl[7][i7];
                          }
                        } /* i7 */
                        mask ^= tmpl[6][i6];
                      }
                    } /* i6 */
                    mask ^= tmpl[5][i5];
                  }
                }  /* i5 */
                mask ^= tmpl[4][i4];
              }
            }  /* i4 */
            mask ^= tmpl[3][i3];
          }
        } /* i3 */
      } /* i2 */
      ct += inner_ct;
      rep2[outer_ct-1][6] += inner_ct;
    }  /* end of solve block */

  } /* end of level == 22 */

  return;
}

/* ----------------------------------------------------------------------
 * PROGRAM START
 */

int main(void) {
  int classnum;
  int i, j, k;

  get_repx(0);
  grand_total_hi = grand_total_lo = 0;
  for (i=0; i<NREPX; i++) colour[i] = i;
  for (i=classnum=0; i<NREPX; i++) {
    if (colour[i] < i) continue;
    printf("%3d : ",++classnum);
    printrep(rep[i]);
    permute(rep[i],i);
    for (j=i,k=0; j<NREPX; j++) if (colour[j] == i) k += rep[j][6];
    printf(" : %4d : ",k);
    solve_rep(rep[i],0,k);
  }

  printf("Grand total = 0x%06x%06x * 362880 * 5184 solutions\n",grand_total_hi,grand_total_lo);

  return 0;
}
