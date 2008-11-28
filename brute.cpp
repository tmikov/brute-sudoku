// Copyright Tzvetan Mikov <tmikov@gmail.com>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>   
#include <stdarg.h>
#include <ctype.h>

#define EMPTY_CELL -1

#define SZ 9

void fatal_error ( const char * msg, ... )
{
  va_list ap; 
  va_start( ap, msg );
  fprintf( stderr, "***error:" );
  vfprintf( stderr, msg, ap );
  fprintf( stderr, "\n" );
  va_end( ap );
  exit( 1 );
}

bool read_puzzle ( FILE * f, int8_t pz[SZ][SZ] )
{
  int8_t * p = &pz[0][0];
  unsigned count = 0;

  while (count < SZ*SZ)
  {
    int ch = getc( f );
    if (ch == EOF)
    {
      if (count == 0)
        return false;
      else 
        fatal_error( "Unexpected EOF in the middle of puzzle" );
    }
    else if (isspace( ch ))
      ;
    else if (ch == '#')
    {
      do
        ch = getc( f );
      while (ch != '\n' && ch != EOF);
    }
    else if (ch == '0')
      fatal_error( "0 is not allowed" );
    else if (ch >= '1' && ch <= '9')
      p[count++] = ch - '1';
    else
      p[count++] = EMPTY_CELL;
  }
  return true;
}

void print_puzzle ( int8_t pz[SZ][SZ] )
{
  for ( unsigned row = 0; row < SZ; ++row )
  {
    if (row != 0 && row%3==0)
      printf("\n");
    for ( unsigned col = 0; col < SZ; ++col )
    {
      if (col != 0 && col%3==0)
        printf( "  " );
      if (pz[row][col] != EMPTY_CELL)
        printf( "%d ", pz[row][col]+1 );
      else
        printf( "_ " );   
    }
    printf( "\n" );
  }
}

typedef uint_fast16_t mask_t;

static inline int GRP ( unsigned row, unsigned col )
{
  return row/3*3+col/3; 
}


#ifdef _GNU_SOURCE
static inline unsigned BSF ( uint32_t n )
{
  uint32_t res;
  __asm__ __volatile__("bsfl %1,%0\n"
                       : "=r" (res) : "r" (n) );
  return res;
}
#else
static inline unsigned BSF ( uint32_t n ) // 3.868
{
  unsigned res = 0;
  while ((n & 1) == 0)
  {
    n >>= 1;
    ++res;
  }
  return res;
}
#endif

mask_t colMask[SZ];
mask_t rowMask[SZ];
mask_t grpMask[SZ];

static bool solve_helper ( int8_t pz[SZ][SZ], unsigned pos )
{
  for(;;)
  {
    if (pos == SZ*SZ)
      return true;
    if (pz[0][pos] == EMPTY_CELL)
      break;
    ++pos;
  }
  unsigned row = pos/SZ;
  unsigned col = pos%SZ;
  unsigned grp = GRP(row,col);

  mask_t m = rowMask[row] | colMask[col] | grpMask[grp];
  while (m != 0x1FF)
  {
    int num = BSF( ~m );

    pz[row][col] = num;
    mask_t tryMask = 1 << num;
    m |= tryMask;
    rowMask[row] |= tryMask;
    colMask[col] |= tryMask;
    grpMask[grp] |= tryMask;

    if (solve_helper( pz, pos+1 ))
      return true;

    rowMask[row] &= ~tryMask;
    colMask[col] &= ~tryMask;
    grpMask[grp] &= ~tryMask;
  }
  pz[row][col] = EMPTY_CELL;
  return false;
}

bool solve ( int8_t pz[SZ][SZ] )
{
  for ( unsigned i = 0; i != SZ; ++i )
    colMask[i] = rowMask[i] = grpMask[i] = 0;

  // Fill the masks
  for ( unsigned row = 0; row != SZ; ++row )
    for ( unsigned col = 0; col != SZ; ++col )
    {
      int c;
      if ( (c = pz[row][col]) != EMPTY_CELL)
      {
        int grp = GRP(row,col);
        mask_t m = 1 << c;

        if (rowMask[row] & m)
          fatal_error( "Digit %d r:%u, c:%u, g:%u duplicated in row", c+1, row+1, col+1, grp+1 );
        if (colMask[col] & m)
          fatal_error( "Digit %d r:%u, c:%u, g:%u duplicated in col", c+1, row+1, col+1, grp+1 );
        if (grpMask[grp] & m)
          fatal_error( "Digit %d r:%u, c:%u, g:%u duplicated in grp", c+1, row+1, col+1, grp+1 );
        rowMask[row] |= m;
        colMask[col] |= m;
        grpMask[grp] |= m;
      }
    }

  return solve_helper( pz, 0 );
}

int main ( void )
{
  int8_t puzzle[SZ][SZ];

  while (read_puzzle( stdin, puzzle ))
  {
    printf("*********************\nProblem:\n" );
    print_puzzle( puzzle );
    if (solve( puzzle ))
    {
      printf("\nSolution:\n" );
      print_puzzle( puzzle );
      // Test the solution
      solve( puzzle );
    }
    else
      fatal_error( "Failed!!!\n" );
    printf( "\n" );
  }
  return 0;
}
