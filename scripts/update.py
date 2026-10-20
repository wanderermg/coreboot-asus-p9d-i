#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""P9D-I BIOS-only update with fresh NVRAM preservation and independent readback."""
from pathlib import Path
import argparse,hashlib,json,os,struct,subprocess,datetime
SIZE=0x800000;BIOS=0x400000;NV_END=0x490000;STORE=0x410000
CHIP='W25Q64BV/W25Q64CV/W25Q64FV'
def digest(b):return hashlib.sha256(b).hexdigest()
def require(condition,message):
 if not condition:raise RuntimeError(message)
def fmap(b):
 require(len(b)==SIZE and b.find(b'__FMAP__')==NV_END,'Unexpected image size/FMAP location')
 h=struct.unpack_from('<8sBBQI32sH',b,NV_END);require(h[1:3]==(1,1) and h[-1]<64,'Unsupported FMAP header')
 result={}
 for i in range(h[-1]):
  offset,size,name,flags=struct.unpack_from('<II32sH',b,NV_END+56+i*42)
  name=name.rstrip(b'\0').decode('ascii');require(name not in result and offset+size<=SIZE,'Invalid FMAP area')
  result[name]=(offset,size)
 require(result.get('SMMSTORE')==(STORE,0x80000),'Unexpected SMMSTORE layout')
 require(result.get('RW_MRC_CACHE')==(BIOS,0x10000),'Unexpected MRC cache layout')
 require(result.get('COREBOOT')==(0x491000,0x36f000),'Unexpected code layout')
 return result

def prepare(template,current,second,descriptor,expected_sha):
 require(digest(template)==expected_sha,'Release image checksum mismatch')
 require(len(current)==len(second)==len(descriptor)==SIZE,'Incorrect backup size')
 require(current[BIOS:]==second[BIOS:],'BIOS changed between backup reads')
 require(descriptor[:0x10000]==template[:0x10000],'Descriptor differs from supported board')
 require(fmap(template)==fmap(current),'Current firmware has incompatible layout')
 image=bytearray(template);image[BIOS:NV_END]=current[BIOS:NV_END]
 require(image[:BIOS]==template[:BIOS] and image[STORE:NV_END]==current[STORE:NV_END],'Image preparation mismatch')
 return bytes(image)

def verify(prepared,before,after,fd_before,fd_after):
 require(len(prepared)==len(before)==len(after)==len(fd_before)==len(fd_after)==SIZE,'Incorrect readback size')
 require(after[BIOS:]==prepared[BIOS:],'BIOS readback differs')
 require(after[BIOS:NV_END]==before[BIOS:NV_END],'NVRAM/cache differs')
 require(fd_after[:0x10000]==fd_before[:0x10000],'Descriptor changed')

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('image',type=Path);ap.add_argument('metadata',type=Path)
 ap.add_argument('--output',type=Path);ap.add_argument('--write',action='store_true',help='Write and verify the BIOS region after preparing backups')
 args=ap.parse_args();require(os.geteuid()==0,'Run as root');os.umask(0o077)
 require(Path('/sys/class/dmi/id/board_name').read_text().strip()=='P9D-I','Different mainboard')
 image=args.image.read_bytes();meta=json.loads(args.metadata.read_text());require(digest(image)==meta['image_sha256'],'Image checksum mismatch')
 out=args.output or Path('/var/tmp/p9di-update-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ'))
 out.mkdir(mode=0o700,parents=False,exist_ok=False)
 base=['flashrom','-p','internal','-c',CHIP,'--ifd']
 def flash(name,*command):
  r=subprocess.run(base+list(command),capture_output=True);(out/(name+'.log')).write_bytes(r.stdout+r.stderr)
  require(r.returncode==0,f'{name} failed; see {out}; no reboot issued')
 def read(name,region):
  path=out/(name+'.'+region+'-selected.rom');flash(name,'-i',region,'-r',str(path));b=path.read_bytes();require(len(b)==SIZE,'Incorrect selected-read container size');return b
 a=read('before1','bios');b=read('before2','bios');fd=read('fd-before','fd')
 prepared=prepare(image,a,b,fd,meta['image_sha256']);path=out/'prepared-preserve-uefi.rom';path.write_bytes(prepared)
 info={'state':'prepared','release_image_sha256':digest(image),'prepared_sha256':digest(prepared),'SMMSTORE_before_sha256':digest(a[STORE:NV_END]),'write_region':'BIOS 0x400000-0x7fffff only','backup_scope':'Selected BIOS/Descriptor reads in 8MiB containers; NOT full-chip ME/SPS backups'}
 (out/'result.json').write_text(json.dumps(info,indent=2)+'\n')
 if not args.write:print('Prepared only:',out);return
 flash('write','-i','bios','--noverify-all','-w',str(path))
 flash('verify','-i','bios','-v',str(path))
 after=read('after','bios');fd_after=read('fd-after','fd');verify(prepared,a,after,fd,fd_after)
 info.update(state='verified',BIOS_readback_exact=True,NVRAM_preserved_exact=True,Descriptor_readback_unchanged=True,ME_SPS_excluded=True)
 (out/'result.json').write_text(json.dumps(info,indent=2)+'\n');print('BIOS-only write verified; no reboot issued:',out)
if __name__=='__main__':main()
