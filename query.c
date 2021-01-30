
qry_t *parse_qry (char *str) {
  char *s = str;
  qry_t *Q = new_vec (0, sizeof(qry_t));
  lowercase(str);
  s += strspn(s," \t\r");
  
  //csub(qry,"\t\r",' ');
  //spaces2space(qry);
  //chop(qry," ");
  
  
  //if (*s == '(') { qry_t *children = parse_query (s, ')'); }
  
}
