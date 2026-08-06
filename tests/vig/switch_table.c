/* switch, in every shape the generator treats differently.
 *
 * Checked against the system C compiler: the same source compiled natively
 * produces the same output.  A dense run becomes a jump table and an indirect
 * jump; a sparse one stays a chain of comparisons, because a table spanning
 * the gap would be mostly default entries.  The density rule decides, and both
 * paths have to give the same answers.
 *
 * A gap inside a run still gets a table: the missing values point at the
 * default label, so the index needs nothing but the lowest case subtracted.
 */
#include <stdio.h>
/* Dense, sparse, gapped, and with a default -- the cases where a table is and
   is not the right shape. */
static int dense(int v){switch(v){case 0:return 10;case 1:return 11;case 2:return 12;
  case 3:return 13;case 4:return 14;case 5:return 15;default:return -1;}}
static int gapped(int v){switch(v){case 0:return 100;case 1:return 101;case 2:return 102;
  case 3:return 103;case 7:return 107;default:return -1;}}
static int sparse(int v){switch(v){case 1:return 1;case 1000:return 2;default:return -1;}}
static int negatives(int v){switch(v){case -3:return 30;case -2:return 20;case -1:return 10;
  case 0:return 0;case 1:return -10;default:return 999;}}
static int nodefault(int v){int r=-77;switch(v){case 2:r=2;break;case 3:r=3;break;
  case 4:r=4;break;case 5:r=5;break;}return r;}
static int fallthru(int v){int r=0;switch(v){case 0:r+=1;case 1:r+=2;case 2:r+=4;
  case 3:r+=8;break;default:r=-1;}return r;}
int main(void){
    int i;
    printf("dense:"); for(i=-1;i<=6;i++) printf("%d ", dense(i)); printf("\n");
    printf("gapped:"); for(i=-1;i<=8;i++) printf("%d ", gapped(i)); printf("\n");
    printf("sparse:%d %d %d\n", sparse(1), sparse(1000), sparse(500));
    printf("neg:"); for(i=-4;i<=2;i++) printf("%d ", negatives(i)); printf("\n");
    printf("nodefault:"); for(i=1;i<=6;i++) printf("%d ", nodefault(i)); printf("\n");
    printf("fallthru:"); for(i=-1;i<=4;i++) printf("%d ", fallthru(i)); printf("\n");
    return 0;
}
