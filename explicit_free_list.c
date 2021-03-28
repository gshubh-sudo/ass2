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
void my_free(char *c); 
int find_big();
int find_small();
int * head, *lhead;

int *curr_size,*max_size,*free_size,*nblocks,*smol_chunk,*big_chunk;
int my_init(void);
void *my_alloc(size_t size);
void insert_list(int *h);
int *find_fit(size_t size);
void coalesce();

#define fhead_size 12
#define max_size 4096-20

int my_init(void){
	// printf("yo beech\n");
	// printf("yo beech\n");
	size_t a=4;
	head=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_ANON|MAP_PRIVATE, -1, 0);
	// printf("yo beech\n");
	curr_size = head+4076/4;
	free_size=curr_size+1;
	nblocks=curr_size+2;
	smol_chunk=curr_size+3;
	big_chunk=curr_size+4;
	// printf("yo beech\n");

	*curr_size=fhead_size;
	*free_size=max_size-*curr_size;
	*nblocks=0;
	*smol_chunk=*free_size;
	*big_chunk=*free_size;
	// printf("yo beech\n");
	// printf("yo beech\n");

	lhead=head;	
	*head = 4096-20-fhead_size;
	// printf("%d",*head);
	//previous
	*(head+1) =-1;
	//next
	*(head+2)=0;
	//allocated or not
	*(head+3)=0;
	//offset
	*(head+4)=0;
	// printf("yo beech\n");
	// int *p = head;
	// printf("%p %p %d\n",(p),(p+1), (int)sizeof(p) );
}

void *my_alloc(size_t size){
	if(size==0){
		return NULL;
	}
	if(size%8 !=0){
		
		return NULL;
	}
	// printf("yo beech\n");
	int *p = find_fit(size);
	// printf("yo beech\n");
	if(p==NULL){
		printf("no block size this big\n");
		return NULL;
	}

	int *h;
	(*nblocks)+=1;


	if(*p -size <= fhead_size){
		printf("this is exec\n");
		*(p+1) == *(p+2);

		*free_size-=(*p);
		*curr_size+=(*p);

		if(lhead==p){
			if(*(p+1)!=-1){
				lhead = head+*(p+1);
			}
			else lhead=NULL;
		}
		*smol_chunk=find_small();
		*big_chunk=find_big();
		return p+2;
	}

	else{
		h= p+2+size/4;
		*h=*p-8-size;
		*(h+1) = *(p+1);
		*(h+2)= *(p+2) + 8/4 + size/4;
		*p = size;
		*(p+1) =*(p+2);

		// printf("happening here\n");
		// printf("%d %d \n", *free_size, *curr_size );
		*free_size-=(*p)+8;
		*curr_size+=(*p)+8;
		// printf("%d %d \n", *free_size, *curr_size );
		

		
		if(lhead==p){
			lhead=h;
		}
		else{
			insert_list(h);
		}

		*big_chunk=find_big();
		*smol_chunk=find_small();
		
		return p+2;
	}

}
void insert_list(int *h){
	printf("shhhs\n");
	if(lhead==NULL){
		lhead=h;
		return;
	}
	int *h1=lhead;

	if(*(h+2)<*(h1+2)){
		*(h+1) = *(h1+2);
		lhead=h;
		return ;
	}

	int *prev=h1;

	while(*(h+2)>*(h1+2) && *(h1+1)!=-1){

		prev=h1;
		h1=head+*(h1+1);
	}
	if(*(h1+1)==-1){
		if(*(h1+2) > *(h+2)){
			*(prev+1) = *(h+2);
			*(h+1) = *(h1+2);
			return;
		}
		else {
			*(h1+1) = *(h+2);
			return;
		}
	}
	*(prev+1) = *(h+2);
	*(h+1) = *(h1+2);
	return;

}

int *find_fit(size_t size){
	int* h = lhead;
	int *h1=NULL;
	if(*h >=size ){
		return h;
	}

	while(*(h+1)>=0){
		h1=h;
		h = head + *(h+1);
		if(*h >=size){
			*(h1+1)=*(h+1);
			return h;
		}
	}
	return NULL;
}


void print_list(){
	int *h = lhead;
	if(lhead ==NULL){
		printf("the list is empty there are no free blocks\n");
		return;
	}
	printf("\n");
	printf("Print the list of blocks\n");
	printf("%d\n",*h );
	printf("%d\n", *(h+1));
	printf("%d\n", *(h+2));

	while(*(h+1)!=-1){
		h=head+*(h+1);
		printf("\n");
		printf("%d\n",*h );
		printf("%d\n", *(h+1));
		printf("%d\n", *(h+2));
	}
	printf("\n");
	printf("\n");
}

void my_free(char *c){
	// printf("inside free\n");
	if(c==NULL){
		printf("dont gimme null\n");
		return;
	}
	int *h = (int *)c;
	h=h-2;
	*h=*h-4;
	*(h+2)=*(h+1);
	*(h+1)=-1;
	*free_size+=*h;
	*curr_size-=*h;
	*nblocks-=1;
	if(h<head){
		return;
	}

	int *h1 = lhead;
	if(lhead ==NULL){
		lhead = h;
		return;
	}

	insert_list(h);
	coalesce();

	*big_chunk=find_big();
	*smol_chunk=find_small();
}
int find_small(){
	*smol_chunk=10000;
	if(lhead ==NULL){
		return 0;
	}
	int *h =lhead;
	if(*h < *smol_chunk){
		*smol_chunk=*h;
	}

	while(*(h+1)!=-1){
		h=head+*(h+1);
		if(*h < *smol_chunk){
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
	int *h =lhead;
	if(lhead ==NULL){
		return 0;
	}
	if(*h > *big_chunk){
		*big_chunk=*h;
	}

	while(*(h+1)!=-1){
		h=head+*(h+1);
		if(*h > *big_chunk){
			*big_chunk=*h;
		}
	}
	return *big_chunk;
}


void coalesce(){
	int *h1=lhead;
	int *h2=NULL;
	while(*(h1+1)!=-1){
		int fz = *(h1+2)+3+*(h1)/4;
		if(fz==*(h1+1)){
			*free_size+=fhead_size;
			*curr_size-=fhead_size;
			h2= head+*(h1+1);
			*h1 = *h1 + *h2 + 12;
			*(h1+1) = *(h2+1);
			continue;
		}
		h1=head + *(h1+1);
	}
	
}
void my_heapinfo(){

	int a, b, c, d, e, f;
	a=max_size;b=*curr_size;c=*free_size;d=*nblocks;e=*smol_chunk;f=*big_chunk;
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

// int main(int argc, char const *argv[])
// {
// 	my_init();
// 	my_heapinfo();
// 	print_list();
// 	printf("ththth\n");
// 	char *big = (char *)my_alloc(16);
// 	char *bigg = (char *)my_alloc(16);
// 	my_free(big);
// 	printf("hashuhsad\n");
// 	my_heapinfo();
// 	print_list();

// 	// char *s = (char *)my_alloc(8);
// 	// if(s==NULL){
// 	// 	printf("booo this allocator\n");
// 	// }
// 	// else{
// 	// 	strcpy(s,"yaayhhh");
// 	// 	printf("%s\n",s );
// 	// }
	

// 	// my_heapinfo();
// 	// print_list();

// 	// char *t = (char *)my_alloc(8);
// 	// if(t==NULL){
// 	// 	printf("booo this allocator\n");
// 	// }
// 	// else{
// 	// 	strcpy(t,"yaayhhh");
// 	// 	printf("%s\n",t );
// 	// }

// 	// my_heapinfo();
// 	// print_list();

// 	// char *y = (char *)my_alloc(8);
// 	// if(s==NULL){
// 	// 	printf("booo this allocator\n");
// 	// }
// 	// else{
// 	// 	strcpy(y,"yaayhhh");
// 	// 	printf("%s\n",y );
// 	// }

// 	// my_heapinfo();
// 	// print_list();

// 	// my_free(s);

// 	// my_heapinfo();
// 	// print_list();

// 	// my_free(y);

// 	// my_heapinfo();
// 	// print_list();

// 	// my_free(t);

// 	// my_heapinfo();
// 	// print_list();
// 	return 0;
// }
