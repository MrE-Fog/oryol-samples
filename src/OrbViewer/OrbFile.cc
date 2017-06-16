//------------------------------------------------------------------------------
//  OrbFile.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include <stdint.h>
#include "OrbFile.h"
#include "Anim/Anim.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Oryol {

//------------------------------------------------------------------------------
bool
OrbFile::HasCharacter() const {
    return !this->Bones.Empty();
}

//------------------------------------------------------------------------------
bool
OrbFile::Parse(const uint8_t* orbFileData, int orbFileSize) {
    const uint8_t* start = orbFileData;
    this->Start = start;
    const uint8_t* end = orbFileData + orbFileSize;

    if ((start + sizeof(OrbHeader)) >= end) return false;
    const OrbHeader* hdr = (const OrbHeader*) start;
    if (hdr->Magic != 'ORB1') return false;

    // setup item array slices
    VertexComps = Slice<OrbVertexComponent>((OrbVertexComponent*)&start[hdr->VertexComponentOffset], hdr->NumVertexComponents);
    if ((const uint8_t*)VertexComps.end() > end) return false;
    ValueProps = Slice<OrbValueProperty>((OrbValueProperty*)&start[hdr->ValuePropOffset], hdr->NumValueProps);
    if ((const uint8_t*)ValueProps.end() > end) return false;
    TexProps = Slice<OrbTextureProperty>((OrbTextureProperty*)&start[hdr->TexturePropOffset], hdr->NumTextureProps);
    if ((const uint8_t*)TexProps.end() > end) return false;
    Materials = Slice<OrbMaterial>((OrbMaterial*)&start[hdr->MaterialOffset], hdr->NumMaterials);
    if ((const uint8_t*)Materials.end() > end) return false;
    Meshes = Slice<OrbMesh>((OrbMesh*)&start[hdr->MeshOffset], hdr->NumMeshes);
    if ((const uint8_t*)Meshes.end() > end) return false;
    Bones = Slice<OrbBone>((OrbBone*)&start[hdr->BoneOffset], hdr->NumBones);
    if ((const uint8_t*)Bones.end() > end) return false;
    Nodes = Slice<OrbNode>((OrbNode*)&start[hdr->NodeOffset], hdr->NumNodes);
    if ((const uint8_t*)Nodes.end() > end) return false;
    AnimKeyComps = Slice<OrbAnimKeyComponent>((OrbAnimKeyComponent*)&start[hdr->AnimKeyComponentOffset], hdr->NumAnimKeyComponents);
    if ((const uint8_t*)AnimKeyComps.end() > end) return false;
    AnimCurves = Slice<OrbAnimCurve>((OrbAnimCurve*)&start[hdr->AnimCurveOffset], hdr->NumAnimCurves);
    if ((const uint8_t*)AnimCurves.end() > end) return false;
    AnimClips = Slice<OrbAnimClip>((OrbAnimClip*)&start[hdr->AnimClipOffset], hdr->NumAnimClips);
    if ((const uint8_t*)AnimClips.end() > end) return false;

    // vertex-, index- and anim data blobs
    if ((start + hdr->VertexDataOffset + hdr->VertexDataSize) > end) return false;
    if ((start + hdr->IndexDataOffset + hdr->IndexDataSize) > end) return false;
    if ((start + hdr->AnimKeyDataOffset + hdr->AnimKeyDataSize) > end) return false;
    if ((start + hdr->StringPoolDataOffset + hdr->StringPoolDataSize) > end) return false;
    VertexDataOffset = hdr->VertexDataOffset;
    VertexDataSize = hdr->VertexDataSize;
    IndexDataOffset = hdr->IndexDataOffset;
    IndexDataSize = hdr->IndexDataSize;
    AnimDataOffset = hdr->AnimKeyDataOffset;
    AnimDataSize = hdr->AnimKeyDataSize;

    // build the string pool
    const char* stringStart = (const char*) (start + hdr->StringPoolDataOffset);
    const char* stringEnd = stringStart;
    const char* stringPoolEnd = stringStart + hdr->StringPoolDataSize;
    if ((const uint8_t*)stringPoolEnd > end) return false;
    while (stringEnd < stringPoolEnd) {
        if (*stringEnd++ == 0) {
            Strings.Add(stringStart);
            stringStart = stringEnd;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
MeshSetup
OrbFile::MakeMeshSetup() const {
    auto setup = MeshSetup::FromData();
    for (const auto& src : VertexComps) {
        VertexLayout::Component dst;
        switch (src.Attr) {
            case OrbVertexAttr::Position:   dst.Attr = VertexAttr::Position; break;
            case OrbVertexAttr::Normal:     dst.Attr = VertexAttr::Normal; break;
            case OrbVertexAttr::TexCoord0:  dst.Attr = VertexAttr::TexCoord0; break;
            case OrbVertexAttr::TexCoord1:  dst.Attr = VertexAttr::TexCoord1; break;
            case OrbVertexAttr::TexCoord2:  dst.Attr = VertexAttr::TexCoord2; break;
            case OrbVertexAttr::TexCoord3:  dst.Attr = VertexAttr::TexCoord3; break;
            case OrbVertexAttr::Tangent:    dst.Attr = VertexAttr::Tangent; break;
            case OrbVertexAttr::Binormal:   dst.Attr = VertexAttr::Binormal; break;
            case OrbVertexAttr::Weights:    dst.Attr = VertexAttr::Weights; break;
            case OrbVertexAttr::Indices:    dst.Attr = VertexAttr::Indices; break;
            case OrbVertexAttr::Color0:     dst.Attr = VertexAttr::Color0; break;
            case OrbVertexAttr::Color1:     dst.Attr = VertexAttr::Color1; break;
            default: break;
        }
        switch (src.Format) {
            case OrbVertexFormat::Float:    dst.Format = VertexFormat::Float; break;
            case OrbVertexFormat::Float2:   dst.Format = VertexFormat::Float2; break;
            case OrbVertexFormat::Float3:   dst.Format = VertexFormat::Float3; break;
            case OrbVertexFormat::Float4:   dst.Format = VertexFormat::Float4; break;
            case OrbVertexFormat::Byte4:    dst.Format = VertexFormat::Byte4; break;
            case OrbVertexFormat::Byte4N:   dst.Format = VertexFormat::Byte4N; break;
            case OrbVertexFormat::UByte4:   dst.Format = VertexFormat::UByte4; break;
            case OrbVertexFormat::UByte4N:  dst.Format = VertexFormat::UByte4N; break;
            case OrbVertexFormat::Short2:   dst.Format = VertexFormat::Short2; break;
            case OrbVertexFormat::Short2N:  dst.Format = VertexFormat::Short2N; break;
            case OrbVertexFormat::Short4:   dst.Format = VertexFormat::Short4; break;
            case OrbVertexFormat::Short4N:  dst.Format = VertexFormat::Short4N; break;
            default: break;
        }
        setup.Layout.Add(dst);
    }
    for (const auto& subMesh : Meshes) {
        setup.AddPrimitiveGroup(PrimitiveGroup(subMesh.FirstIndex, subMesh.NumIndices));
        setup.NumVertices += subMesh.NumVertices;
        setup.NumIndices += subMesh.NumIndices;
    }
    setup.IndicesType = IndexType::Index16;
    setup.VertexDataOffset = VertexDataOffset;
    setup.IndexDataOffset = IndexDataOffset;
    return setup;
}

//------------------------------------------------------------------------------
AnimSkeletonSetup
OrbFile::MakeSkeletonSetup(const StringAtom& name) const {
    AnimSkeletonSetup setup;
    setup.Name = name;
    setup.Bones.Reserve(this->Bones.Size());
    for (const auto& src : this->Bones) {
        setup.Bones.Add();
        auto& dst = setup.Bones.Back();
        dst.Name = this->Strings[src.Name];
        dst.ParentIndex = src.Parent;
        // FIXME: inv bind pose should already be in orb file!
        glm::vec4 t(src.Translate[0], src.Translate[1], src.Translate[2], 1.0f);
        glm::vec3 s(src.Scale[0], src.Scale[1], src.Scale[2]);
        glm::quat r(src.Rotate[3], src.Rotate[0], src.Rotate[1], src.Rotate[2]);
        glm::mat4 m = glm::scale(glm::mat4(), s);
        m = m * glm::mat4_cast(r);
        m[3] = t;
        if (dst.ParentIndex != -1) {
            m = setup.Bones[dst.ParentIndex].BindPose * m;
        }
        dst.InvBindPose = glm::inverse(m);
        dst.BindPose = m;
    }
    return setup;
}

//------------------------------------------------------------------------------
AnimLibrarySetup
OrbFile::MakeAnimLibSetup(const StringAtom& name) const {
    AnimLibrarySetup setup;
    setup.Name = name;
    setup.CurveLayout.Reserve(this->AnimKeyComps.Size());
    for (const auto& src : this->AnimKeyComps) {
        AnimCurveFormat::Enum dst;
        switch (src.KeyFormat) {
            case OrbAnimKeyFormat::Float:   dst = AnimCurveFormat::Float; break;
            case OrbAnimKeyFormat::Float2:  dst = AnimCurveFormat::Float2; break;
            case OrbAnimKeyFormat::Float3:  dst = AnimCurveFormat::Float3; break;
            case OrbAnimKeyFormat::Float4:  dst = AnimCurveFormat::Float4; break;
            case OrbAnimKeyFormat::Quaternion:  dst = AnimCurveFormat::Quaternion; break;
            default: dst = AnimCurveFormat::Invalid; break;
        }
        o_assert_dbg(AnimCurveFormat::Invalid != dst);
        setup.CurveLayout.Add(dst);
    }
    setup.Clips.Reserve(this->AnimClips.Size());
    for (const auto& src : this->AnimClips) {
        auto& dst = setup.Clips.Add();
        dst.Name = this->Strings[src.Name];
        dst.Length = src.Length;
        dst.KeyDuration = src.KeyDuration;
        Slice<OrbAnimCurve> srcCurves = this->AnimCurves.MakeSlice(src.FirstCurve, src.NumCurves);
        dst.Curves.Reserve(srcCurves.Size());
        for (const auto& srcCurve : srcCurves) {
            auto& dstCurve = dst.Curves.Add();
            dstCurve.Static = (-1 == srcCurve.KeyOffset);
            for (int i = 0; i < 4; i++) {
                dstCurve.StaticValue[i] = srcCurve.StaticKey[i];
            }
        }
    }
    return setup;
}

//------------------------------------------------------------------------------
void
OrbFile::CopyAnimKeys(Id animLibId) const {
    Anim::WriteKeys(animLibId, this->Start+this->AnimDataOffset, this->AnimDataSize);
}

} // namespace Oryol

