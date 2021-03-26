#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>

// using a previous pointer as well hoping that it would help in coalescing
typedef struct header{
	size_t size;
	struct header *next;
	struct header *prev;
}header;
void print_list();
int my_init(void);
void *my_alloc(size_t size);
void *find_fit(size_t size);
void my_free(char *c); 
int find_big();
int find_small();
int * head;

int *curr_size,*max_size,*free_size,*nblocks,*smol_chunk,*big_chunk;

int my_init(void){
	printf("yo beech\n");
	size_t a=4;
	head=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_ANON|MAP_PRIVATE, -1, 0);
	max_size = head+4072/4;
	curr_size=max_size+1;
	free_size=max_size+2;
	nblocks=max_size+3;
	smol_chunk=max_size+4;
	big_chunk=max_size+5;
	printf("yo beech\n");

	*max_size=4096-24;
	*curr_size=20;
	*free_size=*max_size-*curr_size;
	*nblocks=0;
	*smol_chunk=*free_size;
	*big_chunk=*free_size;
	printf("yo beech\n");
	
	
	*head = 4096-20-24;
	//previous
	*(head+1) =-1;
	//next
	*(head+2)=-1;
	//allocated or not
	*(head+3)=0;
	//offset
	*(head+4)=0;

	// int *p = head;
	// printf("%p %p %d\n",(p),(p+1), (int)sizeof(p) );
}

void *my_alloc(size_t size){
	if(size%8 !=0){
		printf("enter a multiple of 8\n");
		return NULL;
	}

	int* p = find_fit(size);
	int *h;
	if(p==NULL){
		printf("no blocks exists, choose a smaller size\n");
		return NULL;
	}
	(*nblocks)+=1;

	if((*p)-size<=20){
		*(p+3)==1;
		
		free_size-=(*p);
		curr_size+=(*p);

		if(*p == *smol_chunk){
			*smol_chunk=find_small();
		}

		if(*p== *big_chunk){
			*big_chunk=find_big();
		}
		return (void*)(p+5);
	}
	else{
		
		*(p+3)=1;
		h=p+5+size/4;
		*(h)   =*(p)-size-20;
		*(h+1) = *(p+4);
		*(h+2) =*(p+2);
		*(h+3) =0;
		*(h+4)=*(p+4)+size/4 + 5;
		*p=size;
		*(p+2)=*(h+4);

		*big_chunk=find_big();
		*smol_chunk=*h;
		*free_size-=(*p)+20;
		*curr_size+=(*p)+20;

		return (void *)(p+5);
	}

}
int find_small(){
	*smol_chunk=10000;
	int *h =head;
	if(*h < *smol_chunk && *(h+3)==0){
		*smol_chunk=*h;
	}

	while(*(h+2)!=-1){
		h=head+*(h+2);
		if(*h < *smol_chunk && *(h+3)==0){
			*smol_chunk=*h;
		}
	}
	if(*smol_chunk==10000){
		return 0;
	}
	return *smol_chunk;
}

int find_big(){
	*big_chunk=0;
	int *h =head;
	if(*h > *big_chunk && *(h+3)==0){
		*big_chunk=*h;
	}

	while(*(h+2)!=-1){
		h=head+*(h+2);
		if(*h > *big_chunk && *(h+3)==0){
			*big_chunk=*h;
		}
	}
	return *big_chunk;
}

void print_list(){
	int *h = head;
	printf("\n");
	printf("Print the list of blocks\n");
	printf("%d\n",*h );
	printf("%d\n", *(h+1));
	printf("%d\n", *(h+2));
	printf("%d\n", *(h+3) );

	while(*(h+2)!=-1){
		h=head+*(h+2);
		printf("\n");
		printf("%d\n",*h );
		printf("%d\n", *(h+1));
		printf("%d\n", *(h+2));
		printf("%d\n", *(h+3) );
	}
	printf("\n");
	printf("\n");
}

void *find_fit(size_t size){
	int *h =head;

	while(*h<size || *(h+3)==1){

		if(*(h+2)==-1){
			break;
		}
		
		h = head + *(h+2);
	}
	if(*h <size ||  *(h+3)==1){
		return NULL;
	}
	else return h;

}

void my_free(char *c){
	// printf("problemo1\n");
	int *h = (int*)c;
	h=h-5;

	//modifying heap info
	*free_size+=*h;
	*curr_size-=*h;
	*nblocks-=1;


	int *prev;
	int *next;
	// printf("problemo2\n");
	*(h+3) = 0;
	int flag=0;
	if(*(h+1)!=-1){
		prev= head+ *(h+1);
		if(*(prev+3) ==0){
			// this assigns the free previous block to the next offset of h
			flag=1;
			*(prev+2)=*(h+2);
			*prev = *prev + *h +20;
			if(*(h+2)!=-1){
				next=head+ *(h+2);
				*(next +1) = *(h+1);
			}
			*free_size+=20;
			*curr_size-=20;
		}
	}
	// printf("problemo3\n");
	int *touse;
	if(flag){
		touse=prev;
	}
	else touse=h;

	// printf("problemo4\n");
	if(*(h+2)!=-1){
		next=head+*(h+2);
		if(*(next+3)==0){

			*touse = *touse + *next +20;
			*(touse +2 ) = *(next +2);

			*free_size+=20;
			*curr_size-=20;
		}
	}
	*big_chunk=find_big();
	*smol_chunk=find_small();
	// printf("problemo5\n");
}
void my_clean(){
	munmap(head,4096);
}
void my_heapinfo(){

	int a, b, c, d, e, f;
	a=*max_size;b=*curr_size;c=*free_size;d=*nblocks;e=*smol_chunk;f=*big_chunk;
	// Do not edit below output format
	printf("=== Heap Info ================\n");
	printf("Max Size: %d\n", a);
	printf("Current Size: %d\n", b);
	printf("Free Memory: %d\n", c);
	printf("Blocks allocated: %d\n", d);
	printf("Smallest available chunk: %d\n", e);
	printf("Largest available chunk: %d\n", f);
	printf("==============================\n");
	// Do not edit above output format
	return;

}
char * trial(){
	char *a= my_alloc(8);
	strcpy(a,"this");
	return a;
}
// }
int main(){
	my_init();
	my_heapinfo();
	print_list();
	char *a =my_alloc(8);
	my_heapinfo();
	print_list();
	char *b =my_alloc(8);
	my_heapinfo();
	print_list();
	// char *c = trial();
	// printf("%s\n",c );
	strcpy(a,"yaay");
	printf("%s\n",a );
	// for(int i=0;i<20;i++){
	// 	printf("%d %d %p\n", i, *(head+i), head+i);
	// }
	my_free(b);
	my_heapinfo();
	print_list();
	my_free(a);
	my_heapinfo();
	print_list();
	// for(int i=0;i<20;i++){
	// 	printf("%d %d %p\n", i, *(head+i), head+i);
	// }
	my_clean();
	// for(int i=0;i<20;i++){
	// 	printf("%d %d %p\n", i, *(head+i), head+i);
	// }
	// printf("%d\n", (int)sizeof(char) );
	// char *p = (char*) malloc(100*sizeof(char));
	// (int *)p;
	// *p=1;
	// *(p+1)=2;
	// printf("%d %d\n",*(p),*(p+1) );
}
