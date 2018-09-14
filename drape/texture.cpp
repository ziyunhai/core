#include "drape/texture.hpp"

#include "drape/glextensions_list.hpp"
#include "drape/glfunctions.hpp"
#include "drape/glsl_func.hpp"
#include "drape/utils/gpu_mem_tracker.hpp"

#include "base/math.hpp"

#include "3party/glm/glm/gtx/bit.hpp"

namespace dp
{
Texture::ResourceInfo::ResourceInfo(m2::RectF const & texRect) : m_texRect(texRect) {}

m2::RectF const & Texture::ResourceInfo::GetTexRect() const { return m_texRect; }

Texture::Texture() {}

Texture::~Texture() { Destroy(); }

void Texture::Create(Params const & params)
{
  if (AllocateTexture(params.m_allocator))
    m_hwTexture->Create(params);
}

void Texture::Create(Params const & params, ref_ptr<void> data)
{
  if (AllocateTexture(params.m_allocator))
    m_hwTexture->Create(params, data);
}

void Texture::UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                         ref_ptr<void> data)
{
  ASSERT(m_hwTexture != nullptr, ());
  m_hwTexture->UploadData(x, y, width, height, data);
}

TextureFormat Texture::GetFormat() const
{
  ASSERT(m_hwTexture != nullptr, ());
  return m_hwTexture->GetFormat();
}

uint32_t Texture::GetWidth() const
{
  ASSERT(m_hwTexture != nullptr, ());
  return m_hwTexture->GetWidth();
}

uint32_t Texture::GetHeight() const
{
  ASSERT(m_hwTexture != nullptr, ());
  return m_hwTexture->GetHeight();
}

float Texture::GetS(uint32_t x) const
{
  ASSERT(m_hwTexture != nullptr, ());
  return m_hwTexture->GetS(x);
}

float Texture::GetT(uint32_t y) const
{
  ASSERT(m_hwTexture != nullptr, ());
  return m_hwTexture->GetT(y);
}

uint32_t Texture::GetID() const
{
  ASSERT(m_hwTexture != nullptr, ());
  return m_hwTexture->GetID();
}

void Texture::Bind() const
{
  ASSERT(m_hwTexture != nullptr, ());
  m_hwTexture->Bind();
}

void Texture::SetFilter(TextureFilter filter)
{
  ASSERT(m_hwTexture != nullptr, ());
  m_hwTexture->SetFilter(filter);
}

// static
uint32_t Texture::GetMaxTextureSize()
{
  return static_cast<uint32_t>(GLFunctions::glGetInteger(gl_const::GLMaxTextureSize));
}

// static
bool Texture::IsPowerOfTwo(uint32_t width, uint32_t height)
{
  return glm::isPowerOfTwo(static_cast<int>(width)) && glm::isPowerOfTwo(static_cast<int>(height));
}

void Texture::Destroy() { m_hwTexture.reset(); }

bool Texture::AllocateTexture(ref_ptr<HWTextureAllocator> allocator)
{
  if (allocator != nullptr)
  {
    m_hwTexture = allocator->CreateTexture();
    return true;
  }

  return false;
}
}  // namespace dp