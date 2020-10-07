DOCKER=docker
DOCKERTAG=yaknote
DOCKERFILE_PROD=prod.dockerfile
DOCKERFILE_POC=poc.dockerfile
TMPDIR := $(shell mktemp -d)

MKDIR_P=mkdir -p
CP=cp

all: public

.PHONY: public
public: initrd.gz bzImage yaknote.ko entrypoint.sh
	$(MKDIR_P) ../public 
	$(CP) initrd.gz ../public/initrd.gz
	$(CP) bzImage ../public/bzImage
	$(CP) yaknote.ko ../public/yaknote.ko
	$(CP) entrypoint.sh ../public/run.sh

.PHONY: docker
docker:
	$(DOCKER) build -f $(DOCKERFILE_PROD) -t $(DOCKERTAG) .

.PHONY: docker-poc
docker-poc:
	$(DOCKER) build -f $(DOCKERFILE_POC) -t $(DOCKERTAG):poc .

.PHONY: run
run: docker
	$(DOCKER) run -it --rm $(DOCKERTAG)

.PHONY: clean
clean:
	$(RM) -f *.ko initrd*.gz bzImage

copypasta: payload.gz.b64
	( echo "cd /home/user; base64 -d > p.gz << EOF"; \
		cat - ; \
		echo "EOF"; \
		echo "gunzip p.gz"; \
		echo "chmod +x p"; \
		echo "./p" ) > $@ < $^

bzImage: docker
	CONTAINER=$$($(DOCKER) create $(DOCKERTAG)) \
	&& $(DOCKER) cp $$CONTAINER:/bzImage . \
	&& $(DOCKER) rm $$CONTAINER

initrd.gz: docker
	CONTAINER=$$($(DOCKER) create $(DOCKERTAG)) \
	&& $(DOCKER) cp $$CONTAINER:/initrd.gz . \
	&& $(DOCKER) rm $$CONTAINER

yaknote.ko: initrd.gz
	gzcat $^ | (cd $(TMPDIR); cpio -i)
	cp $(TMPDIR)/$@ $@
	rm -rf $(TMPDIR)

payload.gz.b64: docker-poc
	CONTAINER=$$($(DOCKER) create $(DOCKERTAG):poc) \
	&& $(DOCKER) cp $$CONTAINER:/poc/$@ . \
	&& $(DOCKER) rm $$CONTAINER