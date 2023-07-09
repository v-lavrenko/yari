#!/usr/bin/gawk -f

BEGIN { FS = OFS = "\t"; }

/^##gff/ {next}

{
    if (match($9,"alt=([^;]*);",a)) alt=a[1];
    if (match($9,"ref=([^;]*);",a)) ref=a[1];
    id = $1"_"$4"_"ref"_"alt; src=$2; syn=$3;
    n = split($9,a,"[;=]");
    for (i=1;i<=n;i+=2) print id, a[i], a[i+1];
}

# chr1    clinvar    variant_phenotype    948136    948136    .    +    .    ID=1;clinvar_Accession=RCV000064926;clinvar_AgeOfOnset=N/A;allele_id=6111822;alt=A;clinvar_ClinicalSignificance=not provided;date_last_evaluated=N/A;clinvar_DiseaseName=Malignant melanoma;ensembl=ENSG00000188976,ENSG00000187634;entrez=26155,148398;gene_reviews=N/A;guideline=N/A;hgnc=NOC2L,SAMD11;hgvs=NM_015658.3:c.1654C>T,NP_056473.2:p.Leu552%3D;measure_type=single nucleotide variant;medgen=C0025202;clinvar_MolecularConsequence=synonymous variant;number_submitters=1;omim=N/A;clinvar_Origin=somatic;orpha=N/A;pmid=N/A;clinvar_Prevalence=N/A;ref=G;clinvar_ReviewStatus=not classified by submitter;rsid=rs267598747;uniprot=Q9Y3T9,Q96NU1;feature=NM_015658.3 c.1654C>T%2CNP_056473.2 p.Leu552%3D:Malignant melanoma;hyperlink=http://www.ncbi.nlm.nih.gov/clinvar/RCV000064926
