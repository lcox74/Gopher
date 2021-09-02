# Gopher
> 2nd September 2021

Gopher is a protocol created around 1991 but a team of people at the University
of Minnesota. It's a very light internet protocol that was killed off swiftly 
after the release of the World Wide Web (`www`). It's light in the sense that it
was mainly used for read-only documentation distribution and functioned/looked 
very much like a file system. This allowed Gopher to be compatiable with working
in terminal based computers.

This current repo/project is just a study on the network protocol and trying to
develop a client and server application as a challenge to myself to learn new 
things and see how things were done before my time. The spec I am following for
this project is via the IETF datatracker; 
[RFC 1436](https://datatracker.ietf.org/doc/html/rfc1436). I'll be starting with
a terminal based client application and test it with any servers that still 
actively use the protocol like: `gopher://gopher.floodgap.com/gopher` 
([http proxy](http://gopher.floodgap.com/gopher/gw.lite)).

## How does Gopher work

Unlike `http://` using port `80`, Gopher (`gopher://`) uses port `70` as its 
default port. But similarly still uses TCP for requests. The client and server 
interact as follows:
1. **Client** connects to server (i.e. `gopher.floodgap.com`) on
port `70`.
2. **Server** accepts.
3. **Client** sends a `selector` (i.e. `/gopher`) or just an empty line to see 
what `selectors` the server has.
4. **Server** sends back a response and closes the connection.

The response the server gives back follows a strict format, with each item the
in the response is on a new line (`CRLF`):

| Item Type (single character) | Display String | Selector | Hostname | Port |
| --- | --- | --- | --- | --- |
| `1` | Floodgap Systems gopher root | `/` | `gopher.floodgap.com` | `70` | 
| `0` | Does this gopher menu look correct? | `/gopher/proxy` | `gopher.floodgap.com` | `70` |

Each row is a single item, and each item is made up of `5` different fields each
seperated in the response via a **tab** (`\t`). The last row in the response is
denoted by a single **period** (`.`) saying that there are no more items. The 
following is a filtered raw dump of the request broken down in steps above:

```
1Floodgap Systems gopher root	/	gopher.floodgap.com	70
0Does this gopher menu look correct?	/gopher/proxy	gopher.floodgap.com	70
0A Brief Introduction to Gopherspace -- READ ME FIRST!	/gopher/welcome	gopher.floodgap.com	70
0Why is gopher still relevant?	/gopher/relevance.txt	gopher.floodgap.com	70
0Robert Bigelow: Gopher & Gopherspace	/users/rbigelo/gopher.txt	sdf.org	70
0Wired 08/2010: The Web Is Dead. Long Live The Internet.	/gopher/webisdead.txt	gopher.floodgap.com	70
0The creators of Gopherspace at UMN (Star Tribune 11/5/01)	/gopher/StarTribune.txt	gopher.floodgap.com	70
0Using your web browser to explore Gopherspace	/gopher/wbgopher	gopher.floodgap.com	70
1The Overbite Project: Gopher clients for mobile and desktop (OverbiteWX, OverbiteNX, Overbite Android)	/overbite	gopher.floodgap.com	70
1Patches and clients for other legacy platforms	/gopher/clients	gopher.floodgap.com	70
1The SDF Member PHLOGOSPHERE		phlogosphere.org	70
1The Bucktooth gopher server	/buck	gopher.floodgap.com	70
1The Gopher Toybox: sample gophermaps to create your own server	/gopher/toybox	gopher.floodgap.com	70
1Floodgap Gopher Shortener Service	/gopher/shorten	fld.gp	70
1New gopher servers since 1999	/new	gopher.floodgap.com	70
1Technical specifications, RFCs, documents	/gopher/tech	gopher.floodgap.com	70
0"In The Beginning Was The Command Line"	/gopher/itbwtcl	gopher.floodgap.com	70
.
```

I mention just before that this was a filtered raw dump, I say this because 
there are items that had a non-canonical type denoted as `i` for the item type.
My implementation will only be supporting cannonical types, which are listed 
below:

| Character | Description |
| --- | --- |
| `0` | Text file |
| `1` | Gopher submenu |
| `2` | CCSO Nameserver |
| `3` | Error code returned by a Gopher server to indicate failure |
| `4` | BinHex-encoded file (primarily for Macintosh computers) |
| `5` | DOS file |
| `6` | uuencoded file |
| `7` | Gopher full-text search |
| `8` | Telnet |
| `9` | Binary file |
| `+` | Mirror or alternate server (for load balancing or in case of primary server downtime) |
| `g` | GIF file |
| `I` | Image file |
| `T` | Telnet 3270 |

And thats it, the whole protocol. 