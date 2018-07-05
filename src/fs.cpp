//****************************************************************************//
//								Produção: CHATURA CORPS.					  //
//								    Sr. Peteck								  //
//									Sr. Engers								  //
//																			  //
//																			  //
//																			  //
//																			  //
//****************************************************************************//

#include "fs.h"
#include "disk.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <vector>
#include <cstring>

const unsigned int FS_MAGIC           = 0xf0f03410;
const unsigned int INODES_PER_BLOCK   = 128;
const unsigned int POINTERS_PER_INODE = 5;
const unsigned int POINTERS_PER_BLOCK = 1024;

static bool MOUNTED = false;

std::vector<bool> bitmap;

struct fs_superblock {
	int magic;
	int nblocks;
	int ninodeblocks;
	int ninodes;
};

struct fs_inode {
	int isvalid;
	int size;
	int direct[POINTERS_PER_INODE];
	int indirect;
};

union fs_block {
	struct fs_superblock super;
	struct fs_inode inode[INODES_PER_BLOCK];
	int pointers[POINTERS_PER_BLOCK];
	char data[DISK_BLOCK_SIZE];
};

//class debug



// FUNÇÕES AUXILIARES PARA fs.cpp

bool bitmap_is_valid () {
	return bitmap.size ();
}

void set_bitmap () {
	bitmap.resize (disk_size());
	union fs_block block;
	disk_read (0, block.data);		// recebe o bloco de inodos.


	//Varredura sobre blocos de dados inodos.
	for (int n_inode_block = 0; n_inode_block <= block.super.ninodeblocks; n_inode_block++) {
		bitmap[n_inode_block] = true;

		union fs_block _inode;					// instância de uma unios que será o inodo.
		disk_read (n_inode_block, _inode.data);		// recebe o bloco de inodos.
		// varredura no bloco de inodo
		for (unsigned int _n_inodes = 0; _n_inodes < INODES_PER_BLOCK; _n_inodes++) {

			//verifica se o inodo contém algum dado valido
			if (_inode.inode[_n_inodes].isvalid) {

				//varredura dos inodos dentro do bloco de inodo
				for (unsigned int _direct = 0; _direct < POINTERS_PER_INODE; _direct++) {
					//imprime os valores dos blocos diretos, se houver
					if (_inode.inode[_n_inodes].direct[_direct])	bitmap[_inode.inode[_n_inodes].direct[_direct]] = true;
				}

				// verifica os blocos indiretos, se houver, imprime ele.
				if (_inode.inode[_n_inodes].indirect) {
					bitmap[_inode.inode[_n_inodes].indirect] = true;
					// declada a nova union para acessar os ponteiros indiretos.
					union fs_block _indirect_block;
					// leitura da estrutura de blocos indiretos.
					disk_read (_inode.inode[_n_inodes].indirect,_indirect_block.data);
					// laço para cada bloco de dado indireto sendo imprimido (se houver)
					for (unsigned int _indirect = 0; _indirect < POINTERS_PER_BLOCK; _indirect++) {
						if (_indirect_block.pointers[_indirect]) 	bitmap[_indirect_block.pointers[_indirect]] = true;
					}
				}
			}
		}
	}
	// VERIFICADOR DE bitmap
	for (unsigned int i = 0; i < bitmap.size (); i++)	std::cout << "block :" << i << ". state: " << bitmap[i] << std::endl;
}


//FIM DAS FUNÇÕES AUXILIARES -------------------------------------------


// A função fs_format não precisa necessariamente apagar todos os dados existentes
// basta que a os ponteiros para os blocos de dados sejam desrreferenciados.
// Os ponteiros a serem desrreferenciados serão:
// 	5 ponteiros direct.
// 	1 ponteiro indirect.
// Além de zerar informações dos inodos como:
// 	size = 0;
// 	isvalid = 0;


 // ** FALTA CONDIÇÕES DE FALHA **
//		 - RETORNA FALHA CASO  NAO CONSIGA FORMATAR.

int fs_format () {

	// Imprime os blocos de dados antes de serem formatados
	std::cout << "before format:" << std::endl;
	// se i disco ja estiver montado (bitmap criado e configurado) retorna falha.
	if (bitmap_is_valid ()) return 0;
	/* CASO TIVER QUE ZERAR O bitmap
	union fs_block block;
	disk_read(0,block.data);		// recebe o bloco de inodos.

	// começa a iteração apos os blocos inodos. Limpa todos os blocos subsequentes.
	for(unsigned int i = block.super.ninodeblocks + 1 ; i < bitmap.size() ; i++){
		bitmap[i] = false;
	}
	*/
	//LIMPANDO OS INODE BLOCKS

	//Varredura sobre blocos de dados inodos.
	union fs_block block;
	disk_read (0, block.data);
	for (int n_inode_block = 1; n_inode_block <= block.super.ninodeblocks; n_inode_block ++) {
		union fs_block _inode;					// instância de uma union que será o inodo.
		disk_read(n_inode_block, _inode.data);		// recebe o bloco de inodos do laço.

		// varredura de inodo
		for (unsigned int _n_inodes = 0; _n_inodes < INODES_PER_BLOCK; _n_inodes++) {

			//verifica se o inodo contém algum dado valido, para zerar as suas informações.
			if (_inode.inode[_n_inodes].isvalid){
				_inode.inode[_n_inodes].isvalid = 0;
				_inode.inode[_n_inodes].size = 0;
			//varredura dos inodos dentro do bloco de inodo, para os ponteiros diretos
			for (unsigned int _direct = 0; _direct < POINTERS_PER_INODE; _direct++) {
				// zera as informações dos dados de blocos diretos.
				_inode.inode[_n_inodes].direct[_direct] = 0;
			}
				// zera as informações do dado de bloco indireto.
				_inode.inode[_n_inodes].indirect = 0;
			}
		}
		// escreve no disco as informções modificadas
		disk_write (n_inode_block,_inode.data);
	}
	std::cout << "after format:" << std::endl;
	//imprime o bitmap apos a formatação
	for (unsigned int i = 0 ; i < bitmap.size(); i++)	std::cout << "block :" << i << ". state: " << bitmap[i] << std::endl;
	return 1;
}


void fs_debug () {

	if (MOUNTED == false) {
		std::cout << "Error: please mount first!" << std::endl;
		return;
	}

	union fs_block block;

	disk_read (0, block.data);

	// SUPERBLOCK

	// Condicional para verificar se o numero mágico no superblock é váido.
	// Caso o numero mágico não for vãlido, retorna com erro.
	if (block.super.magic != static_cast <int> (FS_MAGIC)) {
		std::cout << "	magic number is not valid" << std::endl;
		return;
	}
	// Imprime informações acerca do superblock: nblocks,ninodeblocks,ninodes.
	std::cout << "superblock: " << std::endl;
	std::cout << "    magic number is valid" << std::endl;
	std::cout << "    " << block.super.nblocks << " blocks" << std::endl;
	std::cout << "    " << block.super.ninodeblocks << " inode blocks" << std::endl;
	std::cout << "    " << block.super.ninodes << " inodes" << std::endl;

	// INODE BLOCK

	//Varredura sobre blocos de dados inodos.
	for(int n_inode_block = 1; n_inode_block <= block.super.ninodeblocks; n_inode_block++) {
		union fs_block _inode;					// instância de uma unios que será o inodo.
		disk_read(n_inode_block, _inode.data);		// recebe o bloco de inodos.


		// varredura no bloco de inodo
		for (unsigned int _n_inodes = 0; _n_inodes < INODES_PER_BLOCK; _n_inodes++) {



			//verifica se o inodo contém algum dado valido
			if (_inode.inode[_n_inodes].isvalid) {
				std::cout << "inode " << _n_inodes << ":" << std::endl;
				std::cout << "    " <<  "size: " << _inode.inode[_n_inodes].size << " bytes" << std::endl;
				std::cout << "    " <<  "direct blocks: ";

				//varredura dos inodos dentro do bloco de inodo
				for (unsigned int _direct = 0; _direct < POINTERS_PER_INODE; _direct++) {
					//imprime os valores dos blocos diretos, se houver
					if (_inode.inode[_n_inodes].direct[_direct])	std::cout << _inode.inode[_n_inodes].direct[_direct] <<  " ";
				}
				std::cout << std::endl;

				// verifica os blocos indiretos, se houver, imprime ele.
				if (_inode.inode[_n_inodes].indirect) {
					std::cout << "    indirect block: " << _inode.inode[_n_inodes].indirect << std::endl;
					std::cout << "    indirect data blocks: ";
					// declada a nova union para acessar os ponteiros indiretos.
					union fs_block _indirect_block;
					// leitura da estrutura de blocos indiretos.
					disk_read(_inode.inode[_n_inodes].indirect,_indirect_block.data);
					// laço para cada bloco de dado indireto sendo imprimido (se houver)
					for (unsigned int _indirect = 0; _indirect < POINTERS_PER_BLOCK; _indirect++) {
						if (_indirect_block.pointers[_indirect])	std::cout << _indirect_block.pointers[_indirect] <<  " ";
					}
					std::cout << std::endl;
				}
			}
		}
	}
}

// ** FALTA CONDIÇÕES DE FALHA **
// 	- verificar se o disco esta presente.
int fs_mount () {
	union fs_block block;
	disk_read (0, block.data);
	// verifica o numero magico, caso falha, retorna 0 e não faz nada.
	if (block.super.magic != static_cast <int> (FS_MAGIC)) {
		std::cout << "	magic number is not valid" << std::endl;
		return 0;
	}
	// se o bitmap nao for valido (tamanho zero), configura ele.
	if (!bitmap_is_valid ()) set_bitmap ();

	MOUNTED = true;
	return 1;
}

int fs_create () {

	if (MOUNTED == false) {
		std::cout << "Error: please mount first!" << std::endl;
		return -1;
	}

	union fs_block block;

	disk_read (0, block.data);

	//Varredura sobre blocos de dados inodos.
	for(int n_inode_block = 1; n_inode_block <= block.super.ninodeblocks; n_inode_block++) {
		union fs_block _inode;					// instância de uma union que será o inodo.
		disk_read (n_inode_block,_inode.data);		// recebe o bloco de inodos.

		// varredura no bloco de inodo, 0 não pode ser um inodo valido.
		for (unsigned int _n_inodes = 1; _n_inodes < INODES_PER_BLOCK; _n_inodes++) {

			//verifica se o inodo esta vazio(nao for valido)
			if (!_inode.inode[_n_inodes].isvalid) {
				//std::cout << "DEBUG " << n_inode_block << " " << _n_inodes << std::endl;		//debug
				// configurações iniciais zeradas.
				_inode.inode[_n_inodes].isvalid = 1;
				_inode.inode[_n_inodes].size = 0;

				//TROQUEI INODES_PER_BLOCK POR POINTERS_PER_INODE
				for (unsigned int i = 0; i < POINTERS_PER_INODE; i++)	_inode.inode[_n_inodes].direct[i] = 0;
				_inode.inode[_n_inodes].indirect = 0;

				// escreve no disco as informções modificadas
				disk_write (n_inode_block, _inode.data);
				// retorna o numero do inodo criado.
				return _n_inodes;
			}
		}
	}
	// todos os inodos ocupados ou falha.
	return 0;
}

int fs_delete(int inumber) {

	if (MOUNTED == false) {
		std::cout << "Error: please mount first!" << std::endl;
		return -1;
	}

	union fs_block block;

	disk_read (0, block.data);

	union fs_block _inode;	// instância de uma unios que será o inodo.
	//leitura de disco a partir do bloco informado (multiplo de INODES_PER_BLOCK)
	disk_read(1 + (inumber / static_cast <int> (INODES_PER_BLOCK)), _inode.data);		// recebe o bloco de inodos.

	//verifica se o inodo esta vazio(nao for valido)
	if (_inode.inode[inumber % INODES_PER_BLOCK].isvalid) {

		// configurações iniciais zeradas.
		_inode.inode[inumber % INODES_PER_BLOCK].isvalid = 0;
		_inode.inode[inumber % INODES_PER_BLOCK].size = 0;

		//troquei INODES PER BLOCK POR POINTERS PER INODE
		for (unsigned int i = 0; i < POINTERS_PER_INODE; i++)	_inode.inode[inumber % INODES_PER_BLOCK].direct[i] = 0;
		_inode.inode[inumber % INODES_PER_BLOCK].indirect = 0;

		// escreve no disco as informções modificadas
		disk_write (1 + (inumber / static_cast <int> (INODES_PER_BLOCK)), _inode.data);
		// retorna o numero do inodo criado.
		return 1;
	}
	return 0;
}

int fs_getsize (int inumber) {

	//retorna -1 caso nao tenha montado a imagem
	if (MOUNTED == false) {
		std::cout << "Error: please mount first!" << std::endl;
		return -1;
	}

	//recebe o bloco de dados
	union fs_block block;
	disk_read (0, block.data);

	unsigned int _n_blocks = 0;

	//se o inumber eh valido
	if (inumber < block.super.ninodes && block.inode[inumber/INODES_PER_BLOCK].isvalid) {
		union fs_block _inode;

		disk_read (inumber/INODES_PER_BLOCK + 1, _inode.data);

		for (unsigned int _direct = 0; _direct < POINTERS_PER_INODE; _direct++) {
			if (_inode.inode [inumber%INODES_PER_BLOCK].direct[_direct]) _n_blocks++;
		}

		if (_inode.inode[(inumber % static_cast <int> (INODES_PER_BLOCK))].indirect != 0) {
			std::cout << "	piahedaoqujshfdoiuashdoiashd" << std::endl;
			union fs_block _indirect;
			disk_read (_inode.inode[(inumber % static_cast <int> (INODES_PER_BLOCK))].indirect, _indirect.data);
			for (unsigned int n_indirect = 0; n_indirect < POINTERS_PER_BLOCK; n_indirect++) {
				if (_indirect.pointers[n_indirect]) _n_blocks++;
			}
		}
		return _n_blocks*4096;
	}

	return -1;
}


int fs_read (int inumber, char *data, int length, int offset) {

	//caso não esteja montado
	if (MOUNTED == false) {
		std::cout << "Error: please mount first!" << std::endl;
		return -1;
	}

	union fs_block _block;

	disk_read (0, _block.data);
	if (inumber == 0 || inumber > _block.super.ninodes) {
		std::cout << "Erros: inumber invalid!" << std::endl;
		return 0;
	}
	union fs_block _inode;
	disk_read (1 + (inumber/INODES_PER_BLOCK), _inode.data);

	//offset/DISK_BLOCK_SIZE -- numero do bloco inicial
	//offset%DISK_BLOCK_SIZE	-- numero do byte inicial dentro do bloco
	//inumber % INODES_PER_BLOCK -- inodo a ler

	union fs_block _data_block;
	bool perfect_div = false;
	unsigned int number_of_blocks = _inode.inode[inumber%INODES_PER_BLOCK].size/DISK_BLOCK_SIZE;
	if (_inode.inode[inumber%INODES_PER_BLOCK].size%DISK_BLOCK_SIZE == 0) {
		perfect_div = true;
	} else {
		number_of_blocks += 1;
	}
	//numero de bytes que falta ler
	int bytes_remaining = _inode.inode[inumber%INODES_PER_BLOCK].size - offset;
	unsigned int bytes_read = DISK_BLOCK_SIZE;					//numero de bytes lido

	int iterator = 0;			//posição que esta sendo lida
	//int partial_offset = offset%DISK_BLOCK_SIZE;		//offset dentro do bloco de dados depois da primeira vez

	//le os blocos diretos a partir de um bloco inicial calculado pelo offset. Se nao for no diresto, apenas nao entra no for
	for (unsigned int initial_block = offset/DISK_BLOCK_SIZE; initial_block < POINTERS_PER_INODE; initial_block++) {

		if(length <= 0 || bytes_remaining <= 0) break;

		if (_inode.inode[inumber%INODES_PER_BLOCK].direct[initial_block]) {

			if (_inode.inode[inumber%INODES_PER_BLOCK].size < (initial_block + 1)*(DISK_BLOCK_SIZE)) {
				bytes_read = DISK_BLOCK_SIZE - ((initial_block + 1)*(DISK_BLOCK_SIZE) -  _inode.inode[inumber%INODES_PER_BLOCK].size);
			}

			length -= bytes_read;
			bytes_remaining -= bytes_read;

			//le o respectivo bloco direto
			disk_read(_inode.inode[inumber%INODES_PER_BLOCK].direct[initial_block], _data_block.data);

			//copia para buffer o tanto de bytes a ser lido (byte_read)
			std::memcpy(&data[iterator], &_data_block.data[0], bytes_read);

			//atualiza o iterator
			iterator += bytes_read;
		}
	}

	bytes_read = DISK_BLOCK_SIZE;


	if (_inode.inode[inumber%INODES_PER_BLOCK].indirect && (length > 0)) {
			// declada a nova union para acessar os ponteiros indiretos.
			union fs_block _indirect_block;
			// leitura da estrutura de blocos indiretos.
			disk_read (_inode.inode[inumber%INODES_PER_BLOCK].indirect, _indirect_block.data);
			int indirect_initial = 0;

			indirect_initial = ((offset + iterator)/DISK_BLOCK_SIZE - 5);
			for (unsigned int _indirect = indirect_initial; _indirect < indirect_initial + 4; _indirect++) {

				if(length <= 0 || bytes_remaining <= 0) break;

				if (((offset + iterator)/DISK_BLOCK_SIZE == number_of_blocks - 1) && !perfect_div) {
					bytes_read = DISK_BLOCK_SIZE - ((number_of_blocks)*(DISK_BLOCK_SIZE) - (_inode.inode[inumber%INODES_PER_BLOCK].size));
				}

				union fs_block _indirect_data;
				//le o respectivo bloco indireto
				disk_read (_indirect_block.pointers[_indirect], _indirect_data.data);

				std::cout << "indirect: " << _indirect << std::endl;
				//copia para buffer o tanto de bytes a ser lido (byte_read)
				std::memcpy (&data[iterator], &_indirect_data.data[0], bytes_read);

				bytes_remaining -= bytes_read;
				length -= bytes_read;

				//atualiza o iterator
				iterator += bytes_read;
			}
		}

	return iterator;
}

int fs_write (int inumber, const char *data, int length, int offset) {
	//
	// if (MOUNTED == false) {
	// 	std::cout << "Error: please mount first!" << std::endl;
	// 	return -1;
	// }
	//
	union fs_block _block;

	disk_read (0, _block.data);
	if (inumber == 0 || inumber > _block.super.ninodes) {
		std::cout << "Erros: inumber invalid!" << std::endl;
		return 0;
	}

	if (!data) {
		std::cout << "Erro: data invalid" << std::endl;
		return 0;
	}

	bool all_bytes = false;

	if (length%DISK_BLOCK_SIZE == 0) all_bytes = true;

	union fs_block _inode;

	union fs_block _data_block;

	disk_read (1 + (inumber/INODES_PER_BLOCK), _inode.data);

	int iterator = 0;
	int last_block = 0;

	if (offset/DISK_BLOCK_SIZE < 5) {
		for (int i = offset/DISK_BLOCK_SIZE; i < 5; i++) {

			disk_read(_inode.inode[inumber%INODES_PER_BLOCK].direct[i], _data_block.data);
			std::memcpy(&_data_block.data, &data[iterator], DISK_BLOCK_SIZE);
			iterator += DISK_BLOCK_SIZE;
		}
	}






	//union fs_block _data_block;


	return 0;
}
