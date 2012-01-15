/* main function without mpi */	
int
main(int argc, char ** argv)
{
	int i, j, k, rc;
	unsigned char val;
	unsigned int *cid;

	matrix_type *matrix;
	matrix_type *border;
	vector_type *borders;

	struct component *comp_p;
	struct component_list *components;

	if(argc < 2) {
		fprintf(stderr, "To few arguments given.\n\n%s\n",
				usage(argc, argv));
		return -1;
	}

	if(read_input_file(&matrix, argv[1])) {
		fprintf(stderr, "\n%s\n", usage(argc, argv));
		return -1;
	}

	fprintf(stdout, "test_matrix:\n");
	for (i = 0; i < matrix->m; i++) {
		for (j = 0; j < (matrix->n - 1); j++) {
			val = *((unsigned char *) matrix_get(
						matrix, i, j));
			if(val)
				fprintf(stdout, "%2hhd, ", val);
			else
				fprintf(stdout, "  , ");
		}
		val = *((unsigned char *) matrix_get(matrix, i, j));

		if(val)
			fprintf(stdout, "%2hhd\n", val);
		else
			fprintf(stdout, "  \n");
	}

	rc = find_components(matrix, &components, &borders);
	if(rc) {
		fprintf(stderr, "find_components failed.. %s\n", strerror(rc));
		goto err_out;
	}

	fprintf(stdout, "components:\n");
	for(i = 0; i < components->components->elements; i++) {
		comp_p = (struct component *) vector_get_value(
				components->components, i);
		fprintf(stderr, "%u\n", comp_p->size);
	}

	fprintf(stdout, "\nborders:\n");
	for(k = BORDER_MIN; k < BORDER_MAX; k++) {
		border = vector_get_value(borders, k);
		if(!border)
			continue;

		fprintf(stdout, "%s: \n", strborder(k));

		switch(k) {
		case BORDER_TOP:
		case BORDER_BOTTOM:
			for(j = 0, i = 0; j < border->n; j++) {
				cid = (unsigned int *) matrix_get(border, i, j);
				if(!cid)
					fprintf(stdout, "  , ");
				else
					fprintf(stdout, "%2d, ", *cid);
			}
			break;
		case BORDER_LEFT:
		case BORDER_RIGHT:
			for(j = 0, i = 0; i < border->m; i++) {
				cid = (unsigned int *) matrix_get(border, i, j);
				if(!cid)
					fprintf(stdout, "  , ");
				else
					fprintf(stdout, "%2d, ", *cid);
			}
			break;
		}

		fprintf(stdout, "\n");
	}

	component_list_destroy(components);
	matrix_destroy(matrix);
	borders_destroy(borders);

	return rc;
err_out:
	matrix_destroy(matrix);
	return rc;
}

/* working function to distribute in both dimension */
static int mpi_working_function(MPI_Comm *comm, int proc_count, int *dims,
		struct processor_data *pdata)
{
	int i, j, k, rc, found;
	int last = 1, cid = 0;
	int comm_coords[COMM_DIMS], comm_rank, comm_len = 0;
	size_t comm_size = 0;

	MPI_Status status;

	/* local components */
	struct component_list local_comp_list, comm_comp_list, *complistp;
	struct component local_comp, *compp;

	rc = vector_create(&local_comp_list.components, 0, sizeof(struct component));
	if(rc) goto err_out;

	do {
		local_comp.proc_rank = pdata->rank;
		local_comp.component_id = ++cid;
		local_comp.example_coords[0] = urand() % 50 + (pdata->coords[0] * 50);
		local_comp.example_coords[1] = urand() % 50 + (pdata->coords[1] * 50);
		local_comp.size = urand() % 10 + 1;

		rc = vector_add_value(local_comp_list.components, &local_comp);
		if(rc) goto err_out;
	} while((urand() % 100) < 50);

	rc = list_append(pdata->component_lists, &local_comp_list);
	if(rc) goto err_out;

	/*
	 * try to recive something from
	 *
	 * first above, second left neighbour
	 */
	for(i = 0; i < COMM_DIMS; i++) {
		for(j = 0; j < COMM_DIMS; j++) {
			comm_coords[(i + j) % COMM_DIMS] =
				pdata->coords[(i + j) % COMM_DIMS] - (!j ? 1 : 0);
		}

		if(comm_coords[i] < 0)
			continue;

		MPI_Cart_rank(*comm, comm_coords, &comm_rank);

		MPI_Recv(&comm_size, 1, MPI_UNSIGNED, comm_rank,
				MPI_TAG_COMLIST_SIZE, *comm, &status);

		for(k = 0; k < comm_size; k++) {
			MPI_Probe(comm_rank, MPI_TAG_COMLIST, *comm, &status);
			MPI_Get_count(&status, MPI_component_type, &comm_len);

			rc = vector_create(&comm_comp_list.components, comm_len,
					sizeof(struct component));
			if(rc) goto err_out;

			MPI_Recv(comm_comp_list.components->values,
					comm_len, MPI_component_type, comm_rank,
					MPI_TAG_COMLIST, *comm, &status);

			/* set the count of wirtten elements in the vector */
			comm_comp_list.components->elements = comm_len;

			/*
			 * look whether this component_list is already in the list
			 * of component_lists that we know
			 */
			found = 0;
			for(j = 0; j < pdata->component_lists->elements; j++) {
				int rank1, rank2;

				compp = (struct component *) vector_get_value(
							comm_comp_list.components, 0);
				rank1 = compp->proc_rank;

				complistp = (struct component_list *)
					list_element(pdata->component_lists, j);
				compp = (struct component *) vector_get_value(
						complistp->components, 0);
				rank2 = compp->proc_rank;

				if(rank1 == rank2) {
					found = 1;
					break;
				}
			}

			if(!found) {
				rc = list_append(pdata->component_lists,
						&comm_comp_list);
				if(rc) goto err_out;
			} else {
				vector_destroy(comm_comp_list.components);
			}
		}
	}

	/*
	 * try to send to your right and bottom neighbour
	 *
	 * first down, second right
	 */
	for(i = 0; i < COMM_DIMS; i++) {
		for(j = 0; j < COMM_DIMS; j++) {
			comm_coords[(i + j) % COMM_DIMS] =
				pdata->coords[(i + j) % COMM_DIMS] + (!j ? 1 : 0);
		}

		if(comm_coords[i] >= dims[i])
			continue;

		MPI_Cart_rank(*comm, comm_coords, &comm_rank);

		/* send count of component_lists */
		MPI_Send(&pdata->component_lists->elements, 1, MPI_UNSIGNED,
				comm_rank, MPI_TAG_COMLIST_SIZE, *comm);

		for(k = 0; k < pdata->component_lists->elements; k++) {
			list_get(pdata->component_lists, k, &comm_comp_list);

			MPI_Ssend(comm_comp_list.components->values,
					comm_comp_list.components->elements,
					MPI_component_type, comm_rank,
					MPI_TAG_COMLIST, *comm);
		}

		last = 0;
	}

	if(last) {
		fprintf(stdout, "comm finished\nlists on last node:\n");

		for(int i = 0; i < pdata->component_lists->elements; i++) {
			complistp = (struct component_list *)
				list_element(pdata->component_lists, i);
			compp = (struct component *) vector_get_value(
					complistp->components, 0);

			fprintf(stdout, "list from rank %d:\n", compp->proc_rank);

			for(int j = 0; j < complistp->components->elements; j++) {
				compp = (struct component *) vector_get_value(
						complistp->components, j);

				fprintf(stdout, "example_coords: %d, %d; "
						"size: %d; component_id: %hd\n",
						compp->example_coords[0],
						compp->example_coords[1],
						compp->size, compp->component_id);
			}
		}
	}

	k = pdata->component_lists->elements;
	for(i = 0; i < k; i++) {
		list_get_head(pdata->component_lists, &local_comp_list);
		list_remove_head(pdata->component_lists);

		vector_destroy(local_comp_list.components);
	}

	return 0;
err_out:
	MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	fprintf(stderr, "some operation failed with rc.. %d, '%s'\n",
			rc, strerror(rc));
	return rc;
}

