#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
int size = 10;
int arr1[] = {2, 4, 22, 26, 29, 36, 47, 68, 87, 93};
int arr2[] = {8, 9, 11, 51, 56, 64, 82, 84, 88, 97};

    for(int loop = 0; loop < size; loop++)
        printf("%d ", arr1[loop]);
    printf("\n");
    for(int loop = 0; loop < size; loop++)
        printf("%d ", arr2[loop]);

    printf("\n");
    int p = 3;
    int each = size/p;
    if (size%p != 0) each++;
    printf("# of pro:%d each size:%d ",p, each);
    
    printf("\n");
    int max[p-1];
    for (int i = 0; i < p; i++) {
        max[i] = arr1[(i*each)+each-1];
    }
    max[p-1] = arr1[size-1];
    for(int loop = 0; loop < p; loop++)
        printf("%d ", max[loop]);

    printf("\n");
    int a2cut[p-1];
    int count = 0;
    for(int i = 0; i < size; i++) {
        if (arr2[i] >= max[count]) {
            a2cut[count] = i;
            count++;
        }
    }
    for(int loop = 0; loop < p; loop++)
        printf("%d ", a2cut[loop]);
    
}