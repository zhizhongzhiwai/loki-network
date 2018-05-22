#include <arpa/inet.h>
#include <llarp/address_info.h>
#include <llarp/bencode.h>
#include <llarp/mem.h>
#include <llarp/string.h>

static bool
llarp_ai_decode_key(struct dict_reader *r, llarp_buffer_t *key)
{
  int64_t i;
  char tmp[128] = {0};

  llarp_buffer_t strbuf;
  struct llarp_ai *ai = r->user;

  // done
  if(!key)
    return true;

  // rank
  if(llarp_buffer_eq(*key, "c"))
  {
    if(!bdecode_read_integer(r->buffer, &i))
      return false;

    if(i > 65536 || i <= 0)
      return false;

    ai->rank = i;
    return true;
  }

  // dialect
  if(llarp_buffer_eq(*key, "d"))
  {
    if(!bdecode_read_string(r->buffer, &strbuf))
      return false;

    if(strbuf.sz >= sizeof(ai->dialect))
      return false;

    memcpy(ai->dialect, strbuf.base, strbuf.sz);
    ai->dialect[strbuf.sz] = 0;
    return true;
  }

  // encryption public key
  if(llarp_buffer_eq(*key, "e"))
  {
    if(!bdecode_read_string(r->buffer, &strbuf))
      return false;

    if(strbuf.sz != sizeof(llarp_pubkey_t))
      return false;

    memcpy(ai->enc_key, strbuf.base, sizeof(llarp_pubkey_t));
    return true;
  }

  // ip address
  if(llarp_buffer_eq(*key, "i"))
  {
    if(!bdecode_read_string(r->buffer, &strbuf))
      return false;

    if(strbuf.sz >= sizeof(tmp))
      return false;

    memcpy(tmp, strbuf.base, strbuf.sz);
    tmp[strbuf.sz] = 0;

    return inet_pton(AF_INET6, tmp, ai->ip.s6_addr) == 1;
  }

  // port
  if(llarp_buffer_eq(*key, "p"))
  {
    if(!bdecode_read_integer(r->buffer, &i))
      return false;

    if(i > 65536 || i <= 0)
      return false;

    ai->port = i;
    return true;
  }

  // version
  if(llarp_buffer_eq(*key, "v"))
  {
    if(!bdecode_read_integer(r->buffer, &i))
      return false;
    return i == LLARP_PROTO_VERSION;
  }

  // bad key
  return false;
}

bool
llarp_ai_bdecode(struct llarp_ai *ai, llarp_buffer_t *buff)
{
  struct dict_reader reader = {.user = ai, .on_key = &llarp_ai_decode_key};
  return bdecode_read_dict(buff, &reader);
}

bool
llarp_ai_bencode(struct llarp_ai *ai, llarp_buffer_t *buff)
{
  char ipbuff[128] = {0};
  const char *ipstr;
  if(!bencode_start_dict(buff))
    return false;
  /* rank */
  if(!bencode_write_bytestring(buff, "c", 1))
    return false;
  if(!bencode_write_uint16(buff, ai->rank))
    return false;
  /* dialect */
  if(!bencode_write_bytestring(buff, "d", 1))
    return false;
  if(!bencode_write_bytestring(buff, ai->dialect,
                               strnlen(ai->dialect, sizeof(ai->dialect))))
    return false;
  /* encryption key */
  if(!bencode_write_bytestring(buff, "e", 1))
    return false;
  if(!bencode_write_bytestring(buff, ai->enc_key, sizeof(llarp_pubkey_t)))
    return false;
  /** ip */
  ipstr = inet_ntop(AF_INET6, &ai->ip, ipbuff, sizeof(ipbuff));
  if(!ipstr)
    return false;
  if(!bencode_write_bytestring(buff, "i", 1))
    return false;
  if(!bencode_write_bytestring(buff, ipstr, strnlen(ipstr, sizeof(ipbuff))))
    return false;
  /** port */
  if(!bencode_write_bytestring(buff, "p", 1))
    return false;
  if(!bencode_write_uint16(buff, ai->port))
    return false;

  /** version */
  if(!bencode_write_version_entry(buff))
    return false;
  /** end */
  return bencode_end(buff);
}

static bool
llarp_ai_list_iter_bencode(struct llarp_ai_list_iter *iter, struct llarp_ai *ai)
{
  return llarp_ai_bencode(ai, iter->user);
}

bool
llarp_ai_list_bencode(struct llarp_ai_list *l, llarp_buffer_t *buff)
{
  if(!bencode_start_list(buff))
    return false;
  struct llarp_ai_list_iter ai_itr = {.user  = buff,
                                      .visit = &llarp_ai_list_iter_bencode};
  llarp_ai_list_iterate(l, &ai_itr);
  return bencode_end(buff);
}

struct llarp_ai_list_node
{
  struct llarp_ai data;
  struct llarp_ai_list_node *next;
};

struct llarp_ai_list
{
  struct llarp_alloc *mem;
  struct llarp_ai_list_node *root;
};

struct llarp_ai_list *
llarp_ai_list_new(struct llarp_alloc *mem)
{
  struct llarp_ai_list *l = mem->alloc(mem, sizeof(struct llarp_ai_list), 8);
  if(l)
  {
    l->mem  = mem;
    l->root = NULL;
  }
  return l;
}

void
llarp_ai_list_free(struct llarp_ai_list **l)
{
  if(*l)
  {
    struct llarp_alloc *mem        = (*l)->mem;
    struct llarp_ai_list_node *cur = (*l)->root;
    while(cur)
    {
      struct llarp_ai_list_node *tmp = cur->next;
      mem->free(mem, cur);
      cur = tmp;
    }
    mem->free(mem, *l);
    *l = NULL;
  }
}

void
llarp_ai_copy(struct llarp_ai *dst, struct llarp_ai *src)
{
  memcpy(dst, src, sizeof(struct llarp_ai));
}

void
llarp_ai_list_pushback(struct llarp_ai_list *l, struct llarp_ai *ai)
{
  struct llarp_ai_list_node *cur = l->root;
  if(cur)
  {
    // go to the end of the list
    while(cur->next)
      cur = cur->next;

    cur->next = l->mem->alloc(l->mem, sizeof(struct llarp_ai_list_node), 16);
    cur       = cur->next;
  }
  else
  {
    l->root = l->mem->alloc(l->mem, sizeof(struct llarp_ai_list_node), 16);
    cur     = l->root;
  }

  llarp_ai_copy(&cur->data, ai);
  cur->next = 0;
}

void
llarp_ai_list_iterate(struct llarp_ai_list *l, struct llarp_ai_list_iter *itr)
{
  struct llarp_ai_list_node *cur = l->root;
  itr->list                      = l;
  while(cur)
  {
    if(!itr->visit(itr, &cur->data))
      return;
    cur = cur->next;
  };
}

bool
llarp_ai_list_index(struct llarp_ai_list *l, ssize_t idx, struct llarp_ai *dst)
{
  // TODO: implement negative indexes
  if(idx < 0)
    return false;

  struct llarp_ai_list_node *cur = l->root;

  if(!cur)
    return false;

  while(idx && cur)
  {
    cur = cur->next;
    --idx;
  }

  if(cur)
  {
    llarp_ai_copy(dst, &cur->data);
    return true;
  }
  return false;
}

static bool
llarp_ai_list_bdecode_item(struct list_reader *r, bool more)
{
  if(!more)
    return true;
  struct llarp_ai_list *l = r->user;
  struct llarp_ai ai;

  if(!llarp_ai_bdecode(&ai, r->buffer))
    return false;

  llarp_ai_list_pushback(l, &ai);
  return true;
}

bool
llarp_ai_list_bdecode(struct llarp_ai_list *l, llarp_buffer_t *buff)
{
  struct list_reader r = {.user = l, .on_item = &llarp_ai_list_bdecode_item};
  return bdecode_read_list(buff, &r);
}
